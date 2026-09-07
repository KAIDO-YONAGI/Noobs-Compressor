// test_writesorter.cpp — WriteSorter 单元测试（纯手写断言，零外部依赖）
// 覆盖：顺序到达即刷、乱序凑齐才刷、移动语义（零拷贝）、刷后接续下一段、
//       drainAll 停刷出口不丢元素。
#include "../../ThreadPool/WriteSorter.h"

#include <cstdint>
#include <cstdio>
#include <memory>
#include <set>
#include <utility>
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

// 带堆载荷的测试元素：地址不变 = 移动而非拷贝
struct Msg
{
    uint64_t sequence = 0;
    std::unique_ptr<int> payload;

    explicit Msg(int v = 0) : payload(std::make_unique<int>(v)) {}
};

// ① 顺序到达：每条即刻成段，takeInOrder 交出并推进已写前缀
static void testInOrderImmediateFlush()
{
    WriteSorter<Msg> sorter;
    Msg m1(10);
    CHECK(sorter.canWrite(1, std::move(m1)));

    std::vector<Msg> batch = sorter.takeInOrder();
    CHECK(batch.size() == 1);
    CHECK(*batch[0].payload == 10);
    CHECK(sorter.getSize() == 0);

    Msg m2(20);
    CHECK(sorter.canWrite(2, std::move(m2))); // 已写前缀已推进到 1，2 即刻可刷
    sorter.takeInOrder();
}

// ② 乱序到达：凑齐连续段才可刷；takeInOrder 按升序整段交出
static void testOutOfOrderFlushInOrder()
{
    WriteSorter<Msg> sorter;
    Msg m3(30), m1(10), m2(20);
    CHECK(!sorter.canWrite(3, std::move(m3)));
    CHECK(!sorter.canWrite(1, std::move(m1)));
    CHECK(sorter.getSize() == 2);
    CHECK(sorter.getMaxSequence() == 3);
    CHECK(sorter.getMinSequence() == 1);
    CHECK(sorter.canWrite(2, std::move(m2))); // 1,2,3 凑齐

    std::vector<Msg> batch = sorter.takeInOrder();
    CHECK(batch.size() == 3);
    CHECK(*batch[0].payload == 10);
    CHECK(*batch[1].payload == 20);
    CHECK(*batch[2].payload == 30); // 升序
    CHECK(sorter.getSize() == 0);
}

// ③ 移动语义：进表和取出都不拷贝载荷（指针地址保持不变）
static void testMoveNotCopy()
{
    WriteSorter<Msg> sorter;
    Msg m(7);
    const int *original = m.payload.get();
    sorter.canWrite(1, std::move(m));
    std::vector<Msg> batch = sorter.takeInOrder();
    CHECK(batch.size() == 1);
    CHECK(batch[0].payload.get() == original); // 同一块堆内存，全程零拷贝
}

// ④ 刷出接续：整段刷出后已写前缀推进，下一段从 4 起即刻可刷
static void testContiguousRounds()
{
    WriteSorter<Msg> sorter;
    Msg a(1), b(2), c(3);
    CHECK(sorter.canWrite(1, std::move(a)));
    CHECK(sorter.canWrite(2, std::move(b)));
    CHECK(sorter.canWrite(3, std::move(c)));
    CHECK(sorter.takeInOrder().size() == 3);

    Msg d(4);
    CHECK(sorter.canWrite(4, std::move(d))); // 4 恰接在 3 之后
    sorter.takeInOrder();
}

// ⑤ drainAll：滞留元素全部移出、簿记复位（停刷出口）
static void testDrainAll()
{
    WriteSorter<Msg> sorter;
    Msg m5(50), m6(60);
    CHECK(!sorter.canWrite(5, std::move(m5)));
    CHECK(!sorter.canWrite(6, std::move(m6)));

    std::vector<Msg> leftovers = sorter.drainAll();
    CHECK(leftovers.size() == 2);
    // unordered_map 遍历无序，只验证两条都在且载荷完好
    std::set<int> payloads{*leftovers[0].payload, *leftovers[1].payload};
    CHECK((payloads == std::set<int>{50, 60}));
    CHECK(sorter.getSize() == 0);
}

int main()
{
    testInOrderImmediateFlush();
    testOutOfOrderFlushInOrder();
    testMoveNotCopy();
    testContiguousRounds();
    testDrainAll();

    if (failures == 0)
    {
        std::printf("test_writesorter: ALL PASS\n");
        return 0;
    }
    std::printf("test_writesorter: %d FAILURES\n", failures);
    return 1;
}
