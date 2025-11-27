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

using FileCount_uint = uint32_t;    // 文件计数类型
using FileSize_uint = uint64_t;     // 文件大小类型
using FileNameSize_uint = uint16_t; // 文件名长度类型
// 压缩相关配置
using CompressStrategy_uint = uint8_t;     // 压缩策略
using CompressorVersion_uint = uint8_t;    // 压缩器版本
using HeaderOffsetSize_uint = uint8_t;     // 头部偏移大小
using DirectoryOffsetSize_uint = uint32_t; // 目录偏移大小

// 常量改用 constexpr（类型安全）
constexpr uint32_t MagicNum = 0xDEADBEEF; // 文件标识魔数

// 实现分割方案，为分块加密和解压时的分块读取密文做准备
constexpr uint32_t BufferSize = 8192; // 偏移量缓冲需要确保大于文件头大小HeaderSize
// 此处采用软件层动态维护tempOffect来实现，避免了ofstream等文件流的默认缓冲导致依赖文件大小的偏移量读取困难问题
// 分割标准上的偏移量不包含分割标准本身的大小，便于随取随用
// 会在数据区作为首选的管理方案来使用

// 文件协议相关
constexpr const char *HeaderFlag = "0";
constexpr const char *FileFlag = "1";
constexpr const char *SeparatedFlag = "2";
constexpr const uint8_t FlagSize = 1;

constexpr const uint8_t DirectoryrStandardSize_Basic =
    FlagSize +
    sizeof(FileNameSize_uint) +
    // 此处应为变长文件名，无法预先定义,需按情况处理
    sizeof(FileCount_uint);

constexpr const uint8_t FileStandardSize_Basic =
    FlagSize +
    sizeof(FileNameSize_uint) +
    // 此处应为变长文件名，无法预先定义,需按情况处理
    sizeof(FileSize_uint) * 2;

constexpr const uint8_t SeparatedStandardSize =
    FlagSize +
    sizeof(DirectoryOffsetSize_uint);

constexpr const uint8_t HeaderSize =
    sizeof(MagicNum) +                 // 4B
    sizeof(CompressStrategy_uint) +    // 1B
    sizeof(CompressorVersion_uint) +   // 1B
    sizeof(HeaderOffsetSize_uint) +    // 1B
    sizeof(DirectoryOffsetSize_uint) + // 4B
    sizeof(MagicNum);                  // 4B

#endif