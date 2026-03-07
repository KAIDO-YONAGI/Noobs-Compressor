// FileLibaray.h
#pragma once

#include <filesystem> //编译时需要强制链接为静态库
// 关于编译：需要使用普通O3优化级别，且需要开启C++20标准支持（编译器选项 -std=c++20）。此外，确保链接器正确链接了所需的库，如stdc++fs（对于某些编译器）以支持文件系统功能。
// 别开LTO（链接时优化）选项，因为它可能会导致某些符号被错误地优化掉，尤其是在使用了模板或内联函数的情况下。
#include <fstream>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <array>
#include <memory>
// 命名空间
namespace Y_flib
{
    using FileCount = uint32_t;
    using FileSize = uint64_t;
    using FileNameSize = uint32_t;

    using CompressStrategy = uint8_t;
    using CompressorVersion = uint8_t;

    using HeaderOffsetSize = uint8_t;
    using DirectoryOffsetSize = uint64_t;

    using UpSizeOfBuffer = uint32_t;

    using SizeOfMagicNum = uint32_t;
    using SizeOfFlag = uint8_t;

    using IvSize = __uint128_t;

    using DataBlock = std::vector<unsigned char>;
}
// 文件标准相关
enum class FlagType : char // 枚举类，强类型检查
{
    Directory = '0',
    File = '1',
    Separated = '2',
    LogicalRoot = '3',
    SymbolLink = '4'
};
constexpr Y_flib::SizeOfFlag FLAG_SIZE = sizeof(FlagType);

constexpr Y_flib::CompressStrategy STRATEGY = 0; // 策略号

constexpr Y_flib::CompressorVersion VERSION = 0; // 版本号

constexpr Y_flib::SizeOfMagicNum MAGIC_NUM = 0xDEADBEEF; // 文件标识魔数
// 实现分割方案，为分块加密和解压时的分块读取密文做准备
constexpr Y_flib::UpSizeOfBuffer BUFFER_SIZE = 8 * 1024 * 1024;     // 读取的数据块大小，需要确保大于文件头大小HeaderSize和各种文件标准的最大值（可以添加检测以确保ENtry原子性）
constexpr Y_flib::UpSizeOfBuffer HEADER_BUFFER_SIZE = 16 * 1024; // 目录缓冲大小

// 此处采用软件层动态维护tempOffect来实现，避免了因ofstream等文件流的默认缓冲而导致的依赖文件大小的偏移量读取时的同步困难问题。此外，频繁地进行flush()可能导致数据丢失
// 分割标准上的偏移量不包含分割标准本身的大小，便于随取随用
// 会在数据区作为首选的偏移量管理方案来使用，比如按照数据块对象提供的size()方法获取块大小，而不是依赖上述存在更新延迟的文件流提供的size方法

// 注意直接使用sizeof返回的参数进行运算时，小于uint64_t的类型会被自动类型转换为ULL，需要按需强制转换后再参与运算

// 目录标准的基础大小（不含变长的文件名，需要自行维护）
constexpr uint8_t DIRECTORY_STANDARD_SIZE_BASIC =
    FLAG_SIZE +
    sizeof(Y_flib::FileNameSize) +
    // 此行应为变长文件名，无法预先定义,需按情况处理
    sizeof(Y_flib::FileCount);

// 文件标准的基础大小（不含变长的文件名，需要自行维护）
constexpr uint8_t FILE_STANDARD_SIZE_BASIC =
    FLAG_SIZE +
    sizeof(Y_flib::FileNameSize) +
    // 此行应为变长文件名，无法预先定义,需按情况处理
    sizeof(Y_flib::FileSize) * 2;

// 分割标准的基础大小
constexpr uint8_t SEPARATED_STANDARD_SIZE =
    FLAG_SIZE +
    sizeof(Y_flib::DirectoryOffsetSize) +
    sizeof(Y_flib::IvSize);

// 符号链接标准的基础大小
constexpr uint8_t SYMBOL_LINK_STANDARD_SIZE_BASIC =
    FLAG_SIZE +
    sizeof(Y_flib::FileNameSize) +
    sizeof(Y_flib::FileNameSize)
    // 变长文件名
    // 变长文件路径
    ;

// 文件头的大小
#pragma pack(push, 1)
struct Header
{
    Y_flib::SizeOfMagicNum magicNum_1 = 0;
    Y_flib::CompressStrategy strategy = 0;
    Y_flib::CompressorVersion version = 0;
    Y_flib::HeaderOffsetSize headerOffset = 0;
    Y_flib::DirectoryOffsetSize directoryOffset = 0;
    Y_flib::SizeOfMagicNum magicNum_2 = 0;
};
#pragma pack(pop)
constexpr uint8_t HEADER_SIZE = sizeof(Header);
