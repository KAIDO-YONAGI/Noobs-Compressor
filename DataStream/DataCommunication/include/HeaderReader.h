#ifndef HEADERREADER
#define HEADERREADER

#include<fstream>
#include<filesystem>
#include<vector>
#include<iostream>
#include<cstring>
#include<string>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdint>

using namespace std;

namespace fs = std::filesystem; 

void readerForCompression();
void readerForDecompression();
void listFiles(const fs::path &basePath, const fs::path &relativePath, std::vector<std::string> &files);
void appendMagicStatic(const string& outputFilePath);
void outPutAllPaths(string &outPutFilePath, string &filePathToScan);
bool fileIsExist(string &outPutFilePath);
void scanFlow(string &outPutFilePath, string &filePathToScan);

//对POSIX文件方案启用宏定义

#define POSIX_DIR            DIR
#define POSIX_DIRENT         dirent
#define POSIX_STAT           struct stat
#define POSIX_OPENDIR        opendir
#define POSIX_READDIR        readdir
#define POSIX_CLOSEDIR       closedir
#define POSIX_STAT_FUNC      stat
#define POSIX_S_ISDIR(mode)  S_ISDIR(mode)

#endif

