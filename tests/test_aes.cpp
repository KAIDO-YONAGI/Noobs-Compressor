// test_aes.cpp —— AES-128-CFB 模块单元测试（手写断言，零外部依赖）
// 覆盖：round-trip 各种尺寸/模式、IV 前置格式与随机性、实例复用、
//       截断输入抛错、错误密钥、密文扩散、非法 mode。
//
// 构建：见同目录 CMakeLists.txt（MinGW，无 Qt）
// 退出码：0 = 全部通过；非 0 = 失败用例数

#include "My_Aes.h"

#include <cstdio>
#include <cstring>
#include <functional>
#include <random>
#include <string>
#include <vector>

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg)                                            \
    do                                                              \
    {                                                               \
        if (cond)                                                   \
        {                                                           \
            ++g_pass;                                               \
        }                                                           \
        else                                                        \
        {                                                           \
            ++g_fail;                                               \
            std::printf("  [FAIL] %s:%d  %s\n", __FILE__, __LINE__, msg); \
        }                                                           \
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
    case 0: // 全 0
        std::memset(v.data(), 0x00, size);
        break;
    case 1: // 全 FF
        std::memset(v.data(), 0xFF, size);
        break;
    case 2: // 单一字符（高偏斜）
        std::memset(v.data(), 'a', size);
        break;
    case 3: // 顺序值
        for (size_t i = 0; i < size; ++i)
            v[i] = static_cast<unsigned char>(i % 251);
        break;
    case 4: // 伪随机
        for (size_t i = 0; i < size; ++i)
            v[i] = static_cast<unsigned char>(rng() & 0xFF);
        break;
    default: // 高偏斜混合（90% 同字节 + 10% 随机）
        for (size_t i = 0; i < size; ++i)
            v[i] = (rng() % 10 == 0) ? static_cast<unsigned char>(rng() & 0xFF) : 'z';
        break;
    }
    return v;
}

static void roundTripCase(const char *name, const Y_flib::DataBlock &plain,
                          const char *key, std::mt19937 &rng)
{
    Aes aes(key);
    Y_flib::DataBlock encrypted, decrypted;
    aes.doAes(1, plain, encrypted);
    aes.doAes(2, encrypted, decrypted);

    char msg[128];
    std::snprintf(msg, sizeof(msg), "%s: 输出长度应为 输入+16(IV)", name);
    CHECK(encrypted.size() == plain.size() + 16, msg);
    std::snprintf(msg, sizeof(msg), "%s: 解密应还原明文", name);
    CHECK(blocksEqual(plain, decrypted), msg);
    (void)rng;
}

static void testRoundTripSizesAndPatterns()
{
    std::printf("[test] round-trip 尺寸×模式\n");
    std::mt19937 rng(42);

    const size_t MB = 1u << 20;
    const size_t sizes[] = {0, 1, 15, 16, 17, 255, 256, 4095, 4096,
                            65535, MB, 8 * MB - 1, 8 * MB, 8 * MB + 1};
    const int patterns = 6;
    const char *key = "round-trip-key";

    for (size_t size : sizes)
    {
        for (int p = 0; p < patterns; ++p)
        {
            char name[96];
            std::snprintf(name, sizeof(name), "size=%zu pattern=%d", size, p);
            roundTripCase(name, makePattern(size, p, rng), key, rng);
        }
    }
}

static void testIvPrefixAndRandomness()
{
    std::printf("[test] IV 前置与随机性\n");
    Aes aes("iv-test-key");
    Y_flib::DataBlock plain(64, 0x5A);
    Y_flib::DataBlock c1, c2, c3;

    aes.doAes(1, plain, c1);
    aes.doAes(1, plain, c2);
    aes.doAes(1, plain, c3);

    CHECK(c1.size() == plain.size() + 16, "密文长度 = 明文 + 16 字节 IV");

    bool ivAllZero = true;
    for (int i = 0; i < 16; ++i)
        if (c1[i] != 0)
            ivAllZero = false;
    CHECK(!ivAllZero, "IV 不应全 0（构造时清零的 IV 必须被随机值覆盖）");

    CHECK(std::memcmp(c1.data(), c2.data(), 16) != 0, "同明文两次加密 IV 应不同");
    CHECK(!blocksEqual(c1, c2) && !blocksEqual(c2, c3), "同明文两次加密整体密文应不同");

    // 三个密文都能独立解回同一明文（IV 自包含在密文里）
    Y_flib::DataBlock d1, d2, d3;
    aes.doAes(2, c1, d1);
    aes.doAes(2, c2, d2);
    aes.doAes(2, c3, d3);
    CHECK(blocksEqual(plain, d1) && blocksEqual(plain, d2) && blocksEqual(plain, d3),
          "三个随机 IV 的密文都应解回同一明文");
}

static void testInstanceReuse()
{
    std::printf("[test] 同一实例复用 50 轮\n");
    std::mt19937 rng(7);
    Aes aes("reuse-key");
    for (int i = 0; i < 50; ++i)
    {
        Y_flib::DataBlock plain = makePattern(1000 + i * 37, 4, rng);
        Y_flib::DataBlock enc, dec;
        aes.doAes(1, plain, enc);
        aes.doAes(2, enc, dec);
        if (!blocksEqual(plain, dec))
        {
            CHECK(false, "实例复用时第 i 轮 round-trip 失败");
            return;
        }
    }
    CHECK(true, "50 轮复用全部还原");
}

static void testTruncatedInputThrows()
{
    std::printf("[test] 截断输入抛错\n");
    Aes aes("trunc-key");
    Y_flib::DataBlock out;

    for (size_t badSize : {size_t(0), size_t(1), size_t(15)})
    {
        Y_flib::DataBlock tooShort(badSize, 0);
        bool threw = false;
        try
        {
            aes.doAes(2, tooShort, out);
        }
        catch (const std::runtime_error &)
        {
            threw = true;
        }
        CHECK(threw, "解密短于 16 字节的输入应抛 runtime_error");
    }

    bool threwMode = false;
    Y_flib::DataBlock dummy(32, 0);
    try
    {
        aes.doAes(3, dummy, out);
    }
    catch (const std::exception &)
    {
        threwMode = true; // mode 校验抛 invalid_argument（未被包装），短输入抛 runtime_error
    }
    CHECK(threwMode, "非法 mode 应抛异常");
}

static void testWrongKeyGarbage()
{
    std::printf("[test] 错误密钥解密应得到乱码而非明文\n");
    std::mt19937 rng(99);
    Y_flib::DataBlock plain = makePattern(4096, 4, rng);
    Y_flib::DataBlock enc, dec;

    Aes encryptor("correct-horse");
    Aes wrongDecryptor("battery-staple");
    encryptor.doAes(1, plain, enc);
    wrongDecryptor.doAes(2, enc, dec);

    CHECK(!blocksEqual(plain, dec), "错误密钥解密结果不应等于明文");
    CHECK(dec.size() == plain.size(), "错误密钥解密后长度仍应正确");
}

static void testCiphertextDiffusion()
{
    std::printf("[test] 密文扩散（单字节明文差异）\n");
    std::mt19937 rng(1234);
    Y_flib::DataBlock p1 = makePattern(1024, 4, rng);
    Y_flib::DataBlock p2 = p1;
    p2[500] ^= 0x01;

    Aes aes("diffusion-key");
    Y_flib::DataBlock c1, c2, d1, d2;
    aes.doAes(1, p1, c1);
    aes.doAes(2, c1, d1);
    aes.doAes(1, p2, c2);
    aes.doAes(2, c2, d2);

    CHECK(blocksEqual(p1, d1) && blocksEqual(p2, d2), "两份明文各自 round-trip 正确");
    CHECK(std::memcmp(c1.data() + 16, c2.data() + 16, 1024) != 0,
          "单字节明文差异应导致密文体不同");
}

static void testLongKeyVariants()
{
    std::printf("[test] 不同长度密钥\n");
    std::mt19937 rng(5);
    const char *keys[] = {"", "k", "0123456789abcdef",
                          "a-very-long-password-with-lots-of-characters-1234567890"};
    Y_flib::DataBlock plain = makePattern(2048, 3, rng);
    for (const char *key : keys)
    {
        Aes aes(key);
        Y_flib::DataBlock enc, dec;
        aes.doAes(1, plain, enc);
        aes.doAes(2, enc, dec);
        char msg[96];
        std::snprintf(msg, sizeof(msg), "密钥长度 %zu 应 round-trip 正确", std::strlen(key));
        CHECK(blocksEqual(plain, dec), msg);
    }
}

int main()
{
    std::printf("========== test_aes 开始 ==========\n");
    testRoundTripSizesAndPatterns();
    testIvPrefixAndRandomness();
    testInstanceReuse();
    testTruncatedInputThrows();
    testWrongKeyGarbage();
    testCiphertextDiffusion();
    testLongKeyVariants();
    std::printf("========== test_aes 结束: %d 通过, %d 失败 ==========\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
