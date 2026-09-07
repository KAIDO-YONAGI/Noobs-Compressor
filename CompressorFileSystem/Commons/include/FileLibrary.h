// FileLibaray.h
#pragma once
// 本头文件只收录"归档磁盘格式"类型：字段宽度别名、文件/目录/分割标准、
// Header 布局、Constants、BlockSpan。运行期通用类型在 RuntimeLibrary.h（Y_flib::Runtime）。
// 关于编译：需要使用普通O3优化级别，且需要开启C++20标准支持（编译器选项 -std=c++20）。此外，确保链接器正确链接了所需的库，如stdc++fs（对于某些编译器）以支持文件系统功能。
// 别开LTO（链接时优化）选项，因为它可能会导致某些符号被错误地优化掉，尤其是在使用了模板或内联函数的情况下。
#include <cstdint>
#include <array>
#include <vector>
#include "RuntimeLibrary.h"
//关于包含文件的规则
//只在用到头文件的cpp的.h中包含对应头文件，而不是.cpp中，或者总的库头文件中

// 命名空间
namespace Y_flib
{
    // —— 运行期通用类型再导出（定义在 RuntimeLibrary.h 的 Y_flib::Runtime）——
    using Runtime::DataBlock;
    using Runtime::UpSizeOfBuffer;
    using Runtime::MetadataMarker;
    using Runtime::CompressionMode;
    using Runtime::ParserMode;
    using Runtime::AesMode;

    using FileCount = uint32_t;
    using FileSize = uint64_t;
    using FileNameSize = uint32_t;

    using CompressStrategy = uint8_t;
    using CompressorVersion = uint8_t;

    using HeaderOffsetSize = uint8_t;
    using DirectoryOffsetSize = uint64_t;

    // 偏移和长度使用不同的类型名称，杜绝"一型多用"
    using SlotOffset = uint64_t;   // 预留字段在归档中的偏移（绝对位置）
    using BlockLength = uint64_t;  // 数据块长度（分割标准/数据区前缀中的长度字段）

    using SizeOfMagicNum = uint32_t;
    using SizeOfFlag = uint8_t;

    using IvSize = std::array<uint8_t, 16>;

    using ConstSize= uint64_t;

    // 文件标准相关

    enum class FlagType : uint8_t // 枚举类，强类型检查；底层宽度固定 1 字节，是磁盘格式的一部分
    {
        Directory = 0,
        File = 1,
        Separated = 2,
        LogicalRoot = 3,
        SymbolicLinkFile = 4,
        SymbolicLinkDirectory = 5,
        Junction = 6
    };
#pragma pack(push, 1)
    struct Header
    {
        Y_flib::SizeOfMagicNum magicNum_1 = 0;
        Y_flib::CompressStrategy strategy = 0;
        Y_flib::CompressorVersion version = 0;
        Y_flib::HeaderOffsetSize headerOffset = 0;
        Y_flib::DirectoryOffsetSize directoryOffset = 0;
        Y_flib::SizeOfMagicNum magicNum_2 = 0;

        // 内部布局：各预留字段相对文件头起点的位置（按字段顺序累积，
        // 不依赖尾部字段，也不依赖在 Header 之后才定义的 Constants::HEADER_SIZE）
        struct Layout
        {
            static constexpr ConstSize HEADER_OFFSET_FIELD_POS =
                sizeof(SizeOfMagicNum) + sizeof(CompressStrategy) + sizeof(CompressorVersion); // = 6
            static constexpr ConstSize DIRECTORY_OFFSET_FIELD_POS =
                HEADER_OFFSET_FIELD_POS + sizeof(HeaderOffsetSize); // = 7
        };
    };
#pragma pack(pop)

    namespace Constants
    {
        constexpr Y_flib::SizeOfFlag FLAG_SIZE = sizeof(Y_flib::FlagType);
        static_assert(Y_flib::Constants::FLAG_SIZE == 1, "On-disk flag width must stay 1 byte");

        constexpr Y_flib::CompressStrategy STRATEGY = 0; // 策略号

        constexpr Y_flib::CompressorVersion VERSION = 2;               // v2：Windows 链接/Junction 使用独立类型标志
        constexpr Y_flib::CompressorVersion MIN_SUPPORTED_VERSION = 1; // v1 普通文件归档仍可读取

        constexpr Y_flib::SizeOfMagicNum MAGIC_NUM = 0xDEADBEEF; // 文件标识魔数
        // 实现分割方案，为分块加密和解压时的分块读取密文做准备
        constexpr Y_flib::UpSizeOfBuffer BUFFER_SIZE = 8 * 1024 * 1024;  // 读取的数据块大小，需要确保大于文件头大小HeaderSize和各种文件标准的最大值（可以添加检测以确保ENtry原子性）
        constexpr Y_flib::UpSizeOfBuffer HEADER_BUFFER_SIZE = 16 * 1024; // 目录缓冲大小

        // 此处采用软件层动态维护tempOffect来实现，避免了因ofstream等文件流的默认缓冲而导致的依赖文件大小的偏移量读取时的同步困难问题。此外，频繁地进行flush()可能导致数据丢失
        // 分割标准上的偏移量不包含分割标准本身的大小，便于随取随用
        // 会在数据区作为首选的偏移量管理方案来使用，比如按照数据块对象提供的size()方法获取块大小，而不是依赖上述存在更新延迟的文件流提供的size方法

        // 注意直接使用sizeof返回的参数进行运算时，小于uint64_t的类型会被自动类型转换为ULL，需要按需强制转换后再参与运算

        // 目录标准的基础大小（不含变长的文件名，需要自行维护）
        constexpr ConstSize DIRECTORY_STANDARD_SIZE_BASIC =
            FLAG_SIZE +
            sizeof(Y_flib::FileNameSize) +
            // 此行应为变长文件名，无法预先定义,需按情况处理
            sizeof(Y_flib::FileCount);

        // 文件标准的基础大小（不含变长的文件名，需要自行维护）
        constexpr ConstSize FILE_STANDARD_SIZE_BASIC =
            FLAG_SIZE +
            sizeof(Y_flib::FileNameSize) +
            // 此行应为变长文件名，无法预先定义,需按情况处理
            sizeof(Y_flib::FileSize) * 2;

        // 分割标准的基础大小
        constexpr ConstSize SEPARATED_STANDARD_SIZE =
            FLAG_SIZE +
            sizeof(Y_flib::DirectoryOffsetSize) +
            sizeof(Y_flib::IvSize);

        // Windows 链接标准的基础大小；链接类型由 FlagType 保存。
        constexpr ConstSize LINK_STANDARD_SIZE_BASIC =
            FLAG_SIZE +
            sizeof(Y_flib::FileNameSize) +
            sizeof(Y_flib::FileNameSize)
            // 变长文件名
            // 变长文件路径
            ;

        // 文件头的大小
        constexpr ConstSize HEADER_SIZE = sizeof(Header);

        // 交叉验证：Header::Layout 的字段累积形式与 sizeof(Header) 一致
        static_assert(Header::Layout::DIRECTORY_OFFSET_FIELD_POS + sizeof(DirectoryOffsetSize) +
                          sizeof(SizeOfMagicNum) == HEADER_SIZE);

        // 数据块前缀中"块长度字段"的宽度
        constexpr ConstSize BLOCK_LENGTH_FIELD_SIZE = sizeof(DirectoryOffsetSize);

        // IV 的字节数。IvSize 是值类型，位置计算不应依赖 sizeof(值类型)
        constexpr ConstSize IV_BYTES = 16;

    }

#pragma pack(push, 1)
    // 分割标准的前缀布局（磁盘字段顺序：flag + 块长度 + IV）。
    // 只作布局唯一定义与编译期校验；写入路径仍按现有方式逐字段调用
    // writeBinaryStandards，不改为整体写入（避免字节序/填充差异）
    struct SeparatedPrefix
    {
        FlagType flag;
        BlockLength length;
        IvSize iv;
    };

    // 文件标准的尾部布局（原始大小 + 预留的"处理后大小"，紧跟变长文件名之后）
    struct FileStandardTail
    {
        FileSize originalSize;
        FileSize processedSize;
    };
#pragma pack(pop)
    static_assert(sizeof(SeparatedPrefix) == Constants::SEPARATED_STANDARD_SIZE);
    static_assert(sizeof(FileStandardTail) == sizeof(FileSize) * 2);

    /* BlockSpan - 目录数据块在归档中的位置记录（原裸 array<u64,2> 的具名化）
     * 读侧（BinaryStandardLoader）逐块顺手记录，收尾阶段 CatalogFinalizer 原地加密回写使用 */
    struct BlockSpan
    {
        SlotOffset startPos = 0; // 块数据起始绝对偏移
        BlockLength size = 0;    // 块字节数

        /* 块数据之前的 IV 预留槽绝对偏移（紧邻块起点之前 16 字节处） */
        SlotOffset ivSlotPos() const { return startPos - Constants::IV_BYTES; }
    };
}
