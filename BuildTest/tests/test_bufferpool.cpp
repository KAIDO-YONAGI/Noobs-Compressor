// test_bufferpool.cpp —— BufferPool 单元测试（手写断言，零外部依赖）
// 覆盖：借还守恒、容量保留语义（release 只清长度）、空池阻塞与唤醒、
//       close 唤醒等待者并使后续 acquire 抛错、多线程压力下的守恒。

#include "BufferPool.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <set>
#include <thread>
#include <vector>

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg)                                                \
    do                                                                  \
    {                                                                   \
        if (cond)                                                       \
            ++g_pass;                                                   \
        else                                                            \
        {                                                               \
            ++g_fail;                                                   \
            std::printf("  [FAIL] %s:%d  %s\n", __FILE__, __LINE__, msg); \
        }                                                               \
    } while (0)

static void testBorrowReturnConservation()
{
    std::printf("[test] 借还守恒\n");
    Y_flib::BufferPool pool(4, 1u << 20);

    CHECK(pool.capacity() == 4, "容量 = 4");
    CHECK(pool.available() == 4, "初始可用 4");

    std::vector<Y_flib::DataBlock> held;
    std::set<const unsigned char *> pointers;
    for (int i = 0; i < 4; ++i)
    {
        Y_flib::DataBlock b = pool.acquire();
        pointers.insert(b.data());
        held.push_back(std::move(b));
    }
    CHECK(pool.available() == 0, "借空后可用 0");
    CHECK(pointers.size() == 4, "4 个块是不同的缓冲区");

    for (auto &b : held)
        pool.release(std::move(b));
    held.clear();
    CHECK(pool.available() == 4, "全部归还后可用恢复 4");

    // 复用验证：再次借出的应是同批缓冲区
    std::set<const unsigned char *> again;
    for (int i = 0; i < 4; ++i)
    {
        Y_flib::DataBlock b = pool.acquire();
        again.insert(b.data());
        held.push_back(std::move(b));
    }
    for (auto &b : held)
        pool.release(std::move(b));
    int recycled = 0;
    for (auto p : again)
        if (pointers.count(p))
            ++recycled;
    CHECK(recycled == 4, "归还的块被复用（指针同批）");
}

static void testResetSemantics()
{
    std::printf("[test] release 只清长度、保留容量\n");
    const std::size_t blockSize = 1u << 20;
    Y_flib::BufferPool pool(1, blockSize);

    Y_flib::DataBlock b = pool.acquire();
    CHECK(b.size() == 0, "出池块长度为 0");
    CHECK(b.capacity() == blockSize, "出池块容量为块大小（未缩水）");

    b.resize(blockSize / 2); // 模拟借用方填充半块
    for (std::size_t i = 0; i < b.size(); ++i)
        b[i] = static_cast<unsigned char>(i & 0xFF);
    pool.release(std::move(b));

    Y_flib::DataBlock b2 = pool.acquire();
    CHECK(b2.size() == 0, "归还再借出：长度被清 0");
    CHECK(b2.capacity() == blockSize, "归还再借出：容量仍为块大小（未 memset、未缩容）");
    b2.resize(blockSize); // 借用方 resize 到满块
    CHECK(b2.capacity() == blockSize, "resize 到块大小不触发再分配");
    pool.release(std::move(b2));
}

static void testBlockingAndWakeup()
{
    std::printf("[test] 空池阻塞与归还唤醒\n");
    Y_flib::BufferPool pool(1, 64 * 1024);

    Y_flib::DataBlock only = pool.acquire(); // 借走唯一的块

    std::atomic<bool> gotIt{false};
    std::thread waiter([&] {
        Y_flib::DataBlock b = pool.acquire();
        gotIt = true;
        pool.release(std::move(b));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    CHECK(!gotIt.load(), "池空时 acquire 应阻塞");
    CHECK(pool.available() == 0, "阻塞期间可用仍为 0");

    pool.release(std::move(only)); // 归还 → 唤醒 waiter
    waiter.join();
    CHECK(gotIt.load(), "归还后等待者被唤醒并完成借还");
    CHECK(pool.available() == 1, "最终可用恢复 1");
}

static void testCloseWakesWaiters()
{
    std::printf("[test] close 唤醒等待者并拒绝后续 acquire\n");
    Y_flib::BufferPool pool(1, 64 * 1024);

    Y_flib::DataBlock only = pool.acquire();
    pool.release(std::move(only)); // 池内有块

    std::atomic<bool> threw{false};
    Y_flib::DataBlock taken = pool.acquire(); // 先借空
    std::thread waiter([&] {
        try
        {
            Y_flib::DataBlock b = pool.acquire();
            pool.release(std::move(b));
        }
        catch (const std::runtime_error &)
        {
            threw = true;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pool.close(); // 唤醒 waiter → 应抛 runtime_error
    waiter.join();
    CHECK(threw.load(), "close 后等待者被唤醒并收到异常");
    CHECK(pool.closed(), "closed() 状态可见");

    bool threwMain = false;
    try
    {
        Y_flib::DataBlock b = pool.acquire();
        (void)b;
    }
    catch (const std::runtime_error &)
    {
        threwMain = true;
    }
    CHECK(threwMain, "close 后新 acquire 抛 runtime_error");

    pool.release(std::move(taken)); // close 后归还：静默放弃，不崩溃不复活
    CHECK(pool.closed(), "close 后归还不会复活池");
}

static void testMultiThreadStress()
{
    std::printf("[test] 多线程压力（4 线程 × 500 轮）\n");
    const int kThreads = 4;
    const int kRounds = 500;
    Y_flib::BufferPool pool(3, 256 * 1024);

    std::atomic<int> borrowed{0};
    std::atomic<bool> corrupt{false};
    std::vector<std::thread> workers;
    for (int t = 0; t < kThreads; ++t)
    {
        workers.emplace_back([&, t] {
            for (int r = 0; r < kRounds; ++r)
            {
                Y_flib::DataBlock b = pool.acquire();
                b.resize(1024);
                for (std::size_t i = 0; i < b.size(); ++i)
                    b[i] = static_cast<unsigned char>((t + r + i) & 0xFF);
                for (std::size_t i = 0; i < b.size(); ++i)
                    if (b[i] != static_cast<unsigned char>((t + r + i) & 0xFF))
                        corrupt = true;
                ++borrowed;
                pool.release(std::move(b));
            }
        });
    }
    for (auto &w : workers)
        w.join();

    CHECK(borrowed.load() == kThreads * kRounds, "借出总次数守恒");
    CHECK(!corrupt.load(), "借用期间数据不被其他线程篡改");
    CHECK(pool.available() == 3, "结束后全部归还，可用恢复 3");
}

int main()
{
    std::printf("========== test_bufferpool 开始 ==========\n");
    testBorrowReturnConservation();
    testResetSemantics();
    testBlockingAndWakeup();
    testCloseWakesWaiters();
    testMultiThreadStress();
    std::printf("========== test_bufferpool 结束: %d 通过, %d 失败 ==========\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
