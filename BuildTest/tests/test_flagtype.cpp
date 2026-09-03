// test_flagtype.cpp —— FlagType 读写链路单元测试（手写断言，零外部依赖）
// 覆盖：枚举磁盘字节宽度与取值、StandardsWriter/StandardsReader 的 ofstream 与
//       fstream 双通道文件级往返、空分割标准（flag+offset+IV）整条布局往返、
//       DataBlock 缓冲区通道的字节语义。

#include "FileLibrary.h"
#include "ToolClasses.h"

#include <cstdio>
#include <cstring>
#include <fstream>

// 编译期锁定：磁盘上的 flag 宽度必须保持 1 字节
static_assert(sizeof(Y_flib::FlagType) == 1, "FlagType must stay 1 byte on disk");
static_assert(Y_flib::Constants::FLAG_SIZE == 1, "FLAG_SIZE must stay 1");

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

static const char *kTmpFile = "test_flagtype.tmp";

static void testDiskByteValues()
{
    std::printf("[test] 各枚举值的磁盘字节\n");
    const struct
    {
        Y_flib::FlagType flag;
        int expectedByte;
    } cases[] = {
        {Y_flib::FlagType::Directory, 0},
        {Y_flib::FlagType::File, 1},
        {Y_flib::FlagType::Separated, 2},
        {Y_flib::FlagType::LogicalRoot, 3},
        {Y_flib::FlagType::SymbolLink, 4},
    };

    for (const auto &c : cases)
    {
        {
            std::ofstream out(kTmpFile, std::ios::binary | std::ios::trunc);
            Y_flib::StandardsWriter writer;
            writer.writeBinaryStandards(c.flag, out);
        }
        std::ifstream in(kTmpFile, std::ios::binary);
        const int raw = in.get();
        CHECK(raw == c.expectedByte, "flag 落盘字节应为数值 0~4 而非 ASCII '0'~'4'");
        CHECK(in.get() == EOF, "flag 只占 1 字节");
    }
    std::remove(kTmpFile);
}

static void testRoundTripThroughFile()
{
    std::printf("[test] ofstream 通道 5 枚举值顺序往返\n");
    const Y_flib::FlagType flags[] = {
        Y_flib::FlagType::Directory,
        Y_flib::FlagType::File,
        Y_flib::FlagType::Separated,
        Y_flib::FlagType::LogicalRoot,
        Y_flib::FlagType::SymbolLink,
    };

    {
        std::ofstream out(kTmpFile, std::ios::binary | std::ios::trunc);
        Y_flib::StandardsWriter writer;
        for (Y_flib::FlagType f : flags)
            writer.writeBinaryStandards(f, out);
    }
    {
        std::ifstream in(kTmpFile, std::ios::binary);
        Y_flib::StandardsReader reader(in);
        for (Y_flib::FlagType f : flags)
            CHECK(reader.readBinaryStandards<Y_flib::FlagType>() == f, "读回应与写入的枚举值一致");
    }
    std::remove(kTmpFile);
}

static void testSeparatedStandardLayout()
{
    std::printf("[test] 空分割标准整条布局往返（flag + 8B offset + 16B IV）\n");
    const Y_flib::DirectoryOffsetSize offset = 0x1122334455667788ULL;
    const Y_flib::IvSize iv{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}};

    {
        std::ofstream out(kTmpFile, std::ios::binary | std::ios::trunc);
        Y_flib::StandardsWriter writer;
        writer.writeBinaryStandards(Y_flib::FlagType::Separated, out);
        writer.writeBinaryStandards(offset, out);
        writer.writeBinaryStandards(iv, out);
    }

    {
        std::ifstream in(kTmpFile, std::ios::binary);
        in.seekg(0, std::ios::end);
        CHECK(in.tellg() == static_cast<std::streamoff>(Y_flib::Constants::SEPARATED_STANDARD_SIZE),
              "文件大小应等于 SEPARATED_STANDARD_SIZE（1+8+16=25）");
        in.seekg(0, std::ios::beg);

        Y_flib::StandardsReader reader(in);
        CHECK(reader.readBinaryStandards<Y_flib::FlagType>() == Y_flib::FlagType::Separated,
              "分割标准 flag 读回应为 Separated");
        CHECK(reader.readBinaryStandards<Y_flib::DirectoryOffsetSize>() == offset, "offset 读回一致");
        const Y_flib::IvSize ivBack = reader.readBinaryStandards<Y_flib::IvSize>();
        CHECK(ivBack == iv, "IV 读回一致");
    }
    std::remove(kTmpFile);
}

static void testEncryptionSeparatedStandardViaFstream()
{
    std::printf("[test] fstream 通道加密版分割标准往返（flag + 8B offset）\n");
    const Y_flib::DirectoryOffsetSize offset = 0xAABBCCDDEEFF0011ULL;

    {
        std::fstream fs(kTmpFile, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        Y_flib::StandardsWriter writer;
        writer.writeBinaryStandards(Y_flib::FlagType::Separated, fs);
        writer.writeBinaryStandards(offset, fs);
    }

    {
        std::fstream fs(kTmpFile, std::ios::binary | std::ios::in);
        Y_flib::StandardsReader reader(fs);
        CHECK(reader.readBinaryStandards<Y_flib::FlagType>() == Y_flib::FlagType::Separated,
              "fstream 通道 flag 读回应为 Separated");
        CHECK(reader.readBinaryStandards<Y_flib::DirectoryOffsetSize>() == offset,
              "fstream 通道 offset 读回一致");
    }
    std::remove(kTmpFile);
}

static void testBufferChannelBytes()
{
    std::printf("[test] DataBlock 缓冲区通道字节语义\n");
    {
        std::ofstream out(kTmpFile, std::ios::binary | std::ios::trunc);
        Y_flib::StandardsWriter writer;
        writer.writeBinaryStandards(Y_flib::FlagType::Directory, out);
        writer.writeBinaryStandards(Y_flib::FlagType::File, out);
        writer.writeBinaryStandards(Y_flib::FlagType::Separated, out);
        writer.writeBinaryStandards(Y_flib::FlagType::LogicalRoot, out);
        writer.writeBinaryStandards(Y_flib::FlagType::SymbolLink, out);
    }

    // readDataFromReadBlock（EntryParser 私有模板）对 flag 做的是同一件事：
    // 从 unsigned char 缓冲区裸拷 sizeof(FlagType) 字节，这里按相同语义验证
    std::ifstream in(kTmpFile, std::ios::binary);
    Y_flib::DataBlock buffer;
    Y_flib::StandardsReader::readDataBlock(5, in, buffer);
    CHECK(buffer.size() == 5, "缓冲区应读到 5 字节");
    const unsigned char expected[] = {0, 1, 2, 3, 4};
    CHECK(std::memcmp(buffer.data(), expected, 5) == 0, "缓冲区字节应为 0,1,2,3,4");

    for (unsigned i = 0; i < 5; ++i)
    {
        Y_flib::FlagType flag;
        std::memcpy(&flag, buffer.data() + i, sizeof(Y_flib::FlagType));
        CHECK(static_cast<int>(flag) == static_cast<int>(expected[i]), "memcpy 裸拷回枚举应与字节值一致");
    }
    std::remove(kTmpFile);
}

int main()
{
    std::printf("========== test_flagtype 开始 ==========\n");
    testDiskByteValues();
    testRoundTripThroughFile();
    testSeparatedStandardLayout();
    testEncryptionSeparatedStandardViaFstream();
    testBufferChannelBytes();
    std::printf("========== test_flagtype 结束: %d 通过, %d 失败 ==========\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
