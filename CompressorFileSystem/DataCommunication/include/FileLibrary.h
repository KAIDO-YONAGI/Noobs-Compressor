// FileLibaray.h
#pragma once

#include <filesystem> //编译时需要强制链接为静态库

#include <fstream>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <array>
#include <memory>
// 命名空间


using FileCount = uint32_t;    // 文件计数 
using FileSize = uint64_t;     // 文件大小
using FileNameSize = uint32_t; // 文件名长度（也是符号链接的路径长度）
// 压缩相关配置
using CompressStrategy = uint8_t;        // 压缩策略
using CompressorVersion = uint8_t;       // 压缩器版本
using HeaderOffsetSize = uint8_t;        // 头部偏移长度
using DirectoryOffsetSize = uint64_t;    // 目录偏移长度 
using UpSizeOfBuffer = uint32_t;         // 分块的长度
using SizeOfMagicNum = uint32_t;         // 魔数长度
using SizeOfFlag = uint8_t;              // 文件标长度
using IvSize = __uint128_t;              // iv头长度
using DataBlock = std::vector<unsigned char>; // 数据块类型

constexpr CompressStrategy STRATEGY = 0; // 策略号

constexpr CompressorVersion VERSION = 0; // 版本号

constexpr SizeOfMagicNum MAGIC_NUM = 0xDEADBEEF; // 文件标识魔数
// 实现分割方案，为分块加密和解压时的分块读取密文做准备
constexpr UpSizeOfBuffer BUFFER_SIZE = 8 * 1024 * 1024; // 偏移量缓冲需要确保大于文件头大小HeaderSize
constexpr UpSizeOfBuffer DIRECTORY_BUFFER_SIZE = 4 * 1024; // 目录缓冲大小
//TODO:目前的目录分块大小检测仍存在bug，具体来说是边界对齐的时候误以为该块解压解析结束，然而实际上遗漏了边界交界处的文件。
//目前通过提高buffer大小可以降低这种概率
//后续打算通过在分割标准中添加一个标志位来指示是否存在边界交界处的文件，以便正确处理这种情况

// 此处采用软件层动态维护tempOffect来实现，避免了因ofstream等文件流的默认缓冲而导致的依赖文件大小的偏移量读取时的同步困难问题。此外，频繁地进行flush()可能导致数据丢失
// 分割标准上的偏移量不包含分割标准本身的大小，便于随取随用
// 会在数据区作为首选的偏移量管理方案来使用，比如按照数据块对象提供的size()方法获取块大小，而不是依赖上述存在更新延迟的文件流提供的size方法

// 文件标准相关
enum class FlagType : char
{
    Directory = '0',
    File = '1',
    Separated = '2',
    LogicalRoot = '3',
    SymbolLink = '4'
};
constexpr SizeOfFlag FLAG_SIZE = sizeof(FlagType);
constexpr char DIRECTORY_FLAG = static_cast<char>(FlagType::Directory);
constexpr char FILE_FLAG = static_cast<char>(FlagType::File);
constexpr char SEPARATED_FLAG = static_cast<char>(FlagType::Separated);
constexpr char LOGICAL_ROOT_FLAG = static_cast<char>(FlagType::LogicalRoot);
constexpr char SYMBOL_LINK_FLAG = static_cast<char>(FlagType::SymbolLink);
// 注意直接使用sizeof返回的参数进行运算时，小于uint64_t的类型会被自动类型转换为ULL，需要按需强制转换后再参与运算

// 目录标准的基础大小（不含变长的文件名，需要自行维护）
constexpr uint8_t DIRECTORY_STANDARD_SIZE_BASIC =
    FLAG_SIZE +
    sizeof(FileNameSize) +
    // 此行应为变长文件名，无法预先定义,需按情况处理
    sizeof(FileCount);

// 文件标准的基础大小（不含变长的文件名，需要自行维护）
constexpr uint8_t FILE_STANDARD_SIZE_BASIC =
    FLAG_SIZE +
    sizeof(FileNameSize) +
    // 此行应为变长文件名，无法预先定义,需按情况处理
    sizeof(FileSize) * 2;

// 分割标准的基础大小
constexpr uint8_t SEPARATED_STANDARD_SIZE =
    FLAG_SIZE +
    sizeof(DirectoryOffsetSize) +
    sizeof(IvSize);

// 符号链接标准的基础大小
constexpr uint8_t SYMBOL_LINK_STANDARD_SIZE_BASIC =
    FLAG_SIZE +
    sizeof(FileNameSize) +
    sizeof(FileNameSize)
    // 变长文件名
    // 变长文件路径
    ;

// 文件头的大小
#pragma pack(push, 1)
struct Header
{
    SizeOfMagicNum magicNum_1 = 0;
    CompressStrategy strategy = 0;
    CompressorVersion version = 0;
    HeaderOffsetSize headerOffset = 0;
    DirectoryOffsetSize directoryOffset = 0;
    SizeOfMagicNum magicNum_2 = 0;
};
#pragma pack(pop)
constexpr uint8_t HEADER_SIZE =sizeof(Header);
