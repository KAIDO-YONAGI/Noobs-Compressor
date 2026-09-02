// test_huffman.cpp —— Huffman 压缩模块单元测试（手写断言，零外部依赖）
// 覆盖：round-trip 各种尺寸/模式、单字符特例、同一实例连续复用、
//       跨实例输出确定性（验证调用间无状态残留）。

#include "HuffmanCompression.h"

#include <cstdio>
#include <cstring>
#include <random>
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

static bool blocksEqual(const Y_flib::DataBlock &a, const Y_flib::DataBlock &b)
{
    return a.size() == b.size() && std::memcmp(a.data(), b.data(), a.size()) == 0;
}

static Y_flib::DataBlock makePattern(size_t size, int pattern, std::mt19937 &rng)
{
    Y_flib::DataBlock v(size);
    switch (pattern)
    {
    case 0: // 全同字节（单字符 → 特例编码 "0"）
        std::memset(v.data(), 'a', size);
        break;
    case 1: // 两字节交替
        for (size_t i = 0; i < size; ++i)
            v[i] = (i % 2) ? 0xFF : 0x00;
        break;
    case 2: // 顺序值
        for (size_t i = 0; i < size; ++i)
            v[i] = static_cast<unsigned char>(i % 251);
        break;
    case 3: // 伪随机
        for (size_t i = 0; i < size; ++i)
            v[i] = static_cast<unsigned char>(rng() & 0xFF);
        break;
    default: // 高偏斜：90% 同字节 + 10% 随机
        for (size_t i = 0; i < size; ++i)
            v[i] = (rng() % 10 == 0) ? static_cast<unsigned char>(rng() & 0xFF) : 'z';
        break;
    }
    return v;
}

static void roundTripCase(const char *name, const Y_flib::DataBlock &plain, std::mt19937 &rng)
{
    Y_flib::HuffmanCompression hc;
    Y_flib::DataBlock metadata, compressed, decompressed;
    hc.compress(plain, metadata, compressed);
    hc.decompress(metadata, compressed, decompressed, plain.size());

    char msg[128];
    std::snprintf(msg, sizeof(msg), "%s: metadata 非空", name);
    CHECK(!metadata.empty(), msg);
    std::snprintf(msg, sizeof(msg), "%s: 解压应还原原数据", name);
    CHECK(blocksEqual(plain, decompressed), msg);
    (void)rng;
}

static void testRoundTripSizesAndPatterns()
{
    std::printf("[test] round-trip 尺寸×模式\n");
    std::mt19937 rng(42);

    const size_t MB = 1u << 20;
    const size_t sizes[] = {1, 2, 3, 15, 16, 17, 255, 256, 4096, 65536, MB};
    const int patterns = 5;

    for (size_t size : sizes)
        for (int p = 0; p < patterns; ++p)
        {
            char name[96];
            std::snprintf(name, sizeof(name), "size=%zu pattern=%d", size, p);
            roundTripCase(name, makePattern(size, p, rng), rng);
        }
}

static void testSingleCharLargeBlock()
{
    std::printf("[test] 单字符大块（特例编码路径）\n");
    Y_flib::DataBlock plain(200 * 1024, 'q');
    Y_flib::HuffmanCompression hc;
    Y_flib::DataBlock metadata, compressed, decompressed;
    hc.compress(plain, metadata, compressed);
    hc.decompress(metadata, compressed, decompressed, plain.size());
    CHECK(blocksEqual(plain, decompressed), "单字符 200KB 块应还原");
    CHECK(compressed.size() < plain.size() / 8 + 64, "单字符块压缩后约 1 比特/字符（加头部开销）");
}

static void testInstanceReuse()
{
    std::printf("[test] 同一实例连续处理 30 个不同块\n");
    std::mt19937 rng(7);
    Y_flib::HuffmanCompression hc;
    for (int i = 0; i < 30; ++i)
    {
        Y_flib::DataBlock plain = makePattern(5000 + i * 131, (i % 5), rng);
        Y_flib::DataBlock metadata, compressed, decompressed;
        hc.compress(plain, metadata, compressed);
        hc.decompress(metadata, compressed, decompressed, plain.size());
        if (!blocksEqual(plain, decompressed))
        {
            CHECK(false, "实例复用时第 i 轮 round-trip 失败");
            return;
        }
    }
    CHECK(true, "30 轮复用全部还原");
}

static void testDeterminism()
{
    std::printf("[test] 输出确定性（跨实例、重复调用）\n");
    std::mt19937 rng(99);
    Y_flib::DataBlock plain = makePattern(300 * 1024, 3, rng);

    Y_flib::DataBlock m1, c1, m2, c2, m3, c3;
    Y_flib::HuffmanCompression hcA;
    hcA.compress(plain, m1, c1);
    hcA.compress(plain, m2, c2); // 同一实例第二次
    Y_flib::HuffmanCompression hcB;
    hcB.compress(plain, m3, c3); // 另一实例

    CHECK(blocksEqual(m1, m2) && blocksEqual(c1, c2), "同一实例重复压缩输出应一致（无状态残留）");
    CHECK(blocksEqual(m1, m3) && blocksEqual(c1, c3), "不同实例压缩同一输入输出应一致");
}

int main()
{
    std::printf("========== test_huffman 开始 ==========\n");
    testRoundTripSizesAndPatterns();
    testSingleCharLargeBlock();
    testInstanceReuse();
    testDeterminism();
    std::printf("========== test_huffman 结束: %d 通过, %d 失败 ==========\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
