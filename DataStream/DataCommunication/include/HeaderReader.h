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
//对POSIX文件方案启用宏定义

#define POSIX_DIR            DIR
#define POSIX_DIRENT         dirent
#define POSIX_STAT           struct stat
#define POSIX_OPENDIR        opendir
#define POSIX_READDIR        readdir
#define POSIX_CLOSEDIR       closedir
#define POSIX_STAT_FUNC      stat
#define POSIX_S_ISDIR(mode)  S_ISDIR(mode)


namespace fs = std::filesystem; 

class Locator{
    public:
        void relativeLocator(std::ofstream& File,int offset);
        void relativeLocator(std::ifstream& File,int offset);
        void relativeLocator(std::fstream& File, int offset) = delete;//防止发生具有歧义的fstream重载
};

void readerForCompression();
void readerForDecompression();
void appendMagicStatic(const std::string& outputFilePath);
void outPutAllPaths(std::string &outPutFilePath, std::string &filePathToScan);
bool fileIsExist(const std::string &outPutFilePath);
void scanFlow(std::string &outPutFilePath, std::string &filePathToScan);



template <typename T>
void write_binary_le(std::ofstream& file, T value) {
    file.write((const char*)&value, sizeof(T));
}

template <typename T>
T read_binary_le(std::ifstream& file) {
    T value;
    file.read((char*)&value, sizeof(T));
    return value;
}


#endif

