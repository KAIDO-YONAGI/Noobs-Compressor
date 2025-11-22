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
//命名空间

namespace fs = std::filesystem;

// 统一用 using 替代宏定义
using FileCount_uint = uint32_t;          // 文件计数类型
using FileSize_uint = uint64_t;           // 文件大小类型
using FileNameSize_uint = uint16_t;       // 文件名长度类型

// 常量改用 constexpr（类型安全）
constexpr uint32_t MagicNum = 0xDEADBEEF; // 文件标识魔数

// 压缩相关配置
using CompressStrategy_uint = uint8_t;    // 压缩策略
using CompressorVersion_uint = uint8_t;   // 压缩器版本
using HeaderOffsetSize_uint = uint8_t;    // 头部偏移大小
using DirectoryOffsetSize_uint = uint32_t; // 目录偏移大小

#endif