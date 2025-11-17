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
#define FileCount_Int uint32_t
#define FileSize_Int uint64_t
#define FileNameSize_Int uint16_t
#define MagicNum 0xDEADBEEF

#endif