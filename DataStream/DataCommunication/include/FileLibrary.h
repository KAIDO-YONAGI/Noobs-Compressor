// FileLibaray.h
#ifndef FILELIBARAY
#define FILELIBARAY

#include <filesystem> //编译时需要强制链接为静态库

#include <fstream>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <cassert>
//命名空间

namespace fs = std::filesystem;
#define FileCount_uint uint32_t
#define FileSize_uint uint64_t
#define FileNameSize_uint uint16_t
#define MagicNum 0xDEADBEEF
#define CompressStrategy_uint uint8_t
#define CompressorVersion_uint uint8_t
#define HeaderOffsetSize_uint uint8_t
#define DirectoryOffsetSize_uint uint32_t

#endif