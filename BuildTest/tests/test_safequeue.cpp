// test_safequeue.cpp — SafeQueue 单元测试（纯手写断言，零外部依赖）
// 覆盖：FIFO 出货、排空契约（done 后余量照常交付）、关闭后 push 拒绝、
//       空且已关立即返回 false、多生产者多消费者不丢不重。
#include "../../ThreadPool/SafeQueue.h"

#include <atomic>
#include <cstdio>
#include <set>
#include <mutex>
#include <thread>
#include <vector>

static int failures = 0;

#define CHECK(cond)                                                          \
    do                                                                       \
    {                                                                        \
        if (!(cond))                                                          \
        {                                                                     \
            std::fprintf(stderr, "CHECK failed: %s (line %d)\n", #cond, __LINE__); \
            ++failures;                                                       \
        }                                                                     \
    } while (0)

// ① FIFO + 排空契约：setDone 之后已入队元素仍逐件交付，取完才返回 false
static void testFifoAndDrain()
{
    SafeQueue<int> queue;
    CHECK(queue.push(1));
    CHECK(queue.push(2));
    CHECK(queue.push(3));
    queue.setDone();

    int value = 0;
    CHECK(queue.waitPop(value));
    CHECK(value == 1);
    CHECK(queue.waitPop(value));
    CHECK(value == 2);
    CHECK(queue.waitPop(value));
    CHECK(value == 3);
    CHECK(!queue.waitPop(value)); // 空 且 已结束
}

// ② 关闭后 push 拒绝
static void testPushRejectedAfterDone()
{
    SafeQueue<int> queue;
    queue.setDone();
    CHECK(!queue.push(42));

    int value = 0;
    CHECK(!queue.waitPop(value));
}

// ③ 空队列直接 setDone：waitPop 立即返回 false，不挂死
static void testEmptyDoneImmediateFalse()
{
    SafeQueue<int> queue;
    queue.setDone();
    int value = 0;
    CHECK(!queue.waitPop(value));
}

// ④ 多生产者多消费者：不丢不重（4 生产 × 5000 条，4 消费）
static void testMpmcNoLossNoDup()
{
    constexpr int kProducers = 4, kConsumers = 4, kPerProducer = 5000;
    constexpr int kTotal = kProducers * kPerProducer;

    SafeQueue<int> queue;
    std::mutex sinkMutex;
    std::set<int> sink; // 收到的全部值；set 天然去重，size 应等于总数

    std::vector<std::thread> consumers;
    for (int c = 0; c < kConsumers; ++c)
        consumers.emplace_back([&]
                               {
            int value = 0;
            while (queue.waitPop(value))
            {
                std::lock_guard<std::mutex> lock(sinkMutex);
                sink.insert(value);
            } });

    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p)
        producers.emplace_back([&queue, p]
                               {
            for (int i = 0; i < kPerProducer; ++i)
                queue.push(p * kPerProducer + i); });

    for (std::thread &t : producers)
        t.join();
    queue.setDone(); // 生产全部结束后才关闭
    for (std::thread &t : consumers)
        t.join();

    CHECK(sink.size() == static_cast<size_t>(kTotal));
    CHECK(*sink.begin() == 0);
    CHECK(*sink.rbegin() == kTotal - 1);
}

int main()
{
    testFifoAndDrain();
    testPushRejectedAfterDone();
    testEmptyDoneImmediateFalse();
    testMpmcNoLossNoDup();

    if (failures == 0)
    {
        std::printf("test_safequeue: ALL PASS\n");
        return 0;
    }
    std::printf("test_safequeue: %d FAILURES\n", failures);
    return 1;
}
