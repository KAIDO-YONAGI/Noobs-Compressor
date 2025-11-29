// FileLibaray.h
#ifndef FILELIBARAY_H
#define FILELIBARAY_H

#include <filesystem> //编译时需要强制链接为静态库

#include <fstream>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <cassert>

// 命名空间

namespace fs = std::filesystem;

using FileCount_uint = uint32_t;    // 文件计数
using FileSize_uint = uint64_t;     // 文件大小
using FileNameSize_uint = uint16_t; // 文件名长度，部分极端长度文件名可能有超过65535B大小的情况
// 压缩相关配置
using CompressStrategy_uint = uint8_t;     // 压缩策略
using CompressorVersion_uint = uint8_t;    // 压缩器版本
using HeaderOffsetSize_uint = uint8_t;     // 头部偏移长度
using DirectoryOffsetSize_uint = uint32_t; // 目录偏移长度
using UpSizeOfBuffer_uint = uint32_t;      // 分块的长度
using SizeOfMagicNum_uint = uint32_t;      // 魔数长度
using SizeOfFlag_uint = uint8_t;           // 文件标长度
using IvSize_uint = __uint128_t;           // iv头长度

// 常量改用 constexpr（类型安全）
constexpr SizeOfMagicNum_uint MagicNum = 0xDEADBEEF; // 文件标识魔数

// 实现分割方案，为分块加密和解压时的分块读取密文做准备
constexpr UpSizeOfBuffer_uint BufferSize = 8192; // 偏移量缓冲需要确保大于文件头大小HeaderSize
// 此处采用软件层动态维护tempOffect来实现，避免了因ofstream等文件流的默认缓冲而导致的依赖文件大小的偏移量读取时的同步困难问题。此外，频繁地进行flush()可能导致数据丢失
// 分割标准上的偏移量不包含分割标准本身的大小，便于随取随用
// 会在数据区作为首选的偏移量管理方案来使用，比如按照数据块对象提供的size()方法获取块大小，而不是依赖上述存在更新延迟的文件流提供的size方法

// 文件协议相关
constexpr const SizeOfFlag_uint FlagSize = 1;
constexpr const char *HeaderFlag = "0";
constexpr const char *FileFlag = "1";
constexpr const char *SeparatedFlag = "2";

// 注意直接使用sizeof返回的参数进行运算时，小于uint64_t的类型会被自动类型转换为ULL，需要按需强制转换后再参与运算

// 目录标准的基础大小（不含变长的文件名，需要自行维护）
constexpr const uint8_t DirectoryrStandardSize_Basic =
    FlagSize +
    sizeof(FileNameSize_uint) +
    // 此行应为变长文件名，无法预先定义,需按情况处理
    sizeof(FileCount_uint);

// 文件标准的基础大小（不含变长的文件名，需要自行维护）
constexpr const uint8_t FileStandardSize_Basic =
    FlagSize +
    sizeof(FileNameSize_uint) +
    // 此行应为变长文件名，无法预先定义,需按情况处理
    sizeof(FileSize_uint) * 2;

// 分割标准的基础大小
constexpr const uint8_t SeparatedStandardSize =
    FlagSize +
    sizeof(DirectoryOffsetSize_uint) +
    sizeof(IvSize_uint);

// 文件头的大小
constexpr const uint8_t HeaderSize =
    sizeof(MagicNum) +                 // 4B
    sizeof(CompressStrategy_uint) +    // 1B
    sizeof(CompressorVersion_uint) +   // 1B
    sizeof(HeaderOffsetSize_uint) +    // 1B
    sizeof(DirectoryOffsetSize_uint) + // 4B
    sizeof(MagicNum);                  // 4B

#endif