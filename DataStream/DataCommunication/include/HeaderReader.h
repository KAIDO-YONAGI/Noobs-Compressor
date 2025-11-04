#ifndef HEADERREADER
#define HEADERREADER

#include <fstream>
#include <filesystem>
#include <vector>
#include <iostream>
#include <cstring>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdint>

namespace fs = std::filesystem;
//对POSIX文件方案启用宏定义

#define POSIX_DIR DIR
#define POSIX_DIRENT dirent
#define POSIX_STAT struct stat
#define POSIX_OPENDIR opendir
#define POSIX_READDIR readdir
#define POSIX_CLOSEDIR closedir
#define POSIX_STAT_FUNC stat
#define POSIX_S_ISDIR(mode) S_ISDIR(mode)

namespace fs = std::filesystem;

class Locator
{
public:
    void relativeLocator(std::ofstream &File, int offset);
    void relativeLocator(std::ifstream &File, int offset);
    void relativeLocator(std::fstream &File, int offset) = delete; //防止发生具有歧义的fstream重载
};
class FilePath
{
private:
    const char *outPutFilePath;
    const char *filePathToScan;

public:
    FilePath(const char *outPutFilePath, const char *filePathToScan)
    {
        this->outPutFilePath = outPutFilePath;
        this->filePathToScan = filePathToScan;
    }
    const char *getOutPutFilePath() const { return outPutFilePath; }
    const char *getFilePathToScan() const { return filePathToScan; }
};
class FileDetails
{
private:
    std::string name;
    std::string fullPath;

    uint8_t sizeOfName;
    uint64_t fileSize;
    bool isFile;

public:
    const std::string &getName() const { return name; }
    const std::string &getFullPath() const { return fullPath; }

    uint8_t getSizeOfName() const { return sizeOfName; }

    uint64_t getFileSize() const { return fileSize; }

    bool getIsFile() const { return isFile; }

    FileDetails(std::string name, uint8_t sizeOfName, uint64_t fileSize, bool isFile, std::string fullPath)
    {
        this->name = name;
        this->sizeOfName = sizeOfName;
        this->fileSize = fileSize;
        this->isFile = isFile;
        this->fullPath = fullPath;
    }
};
class BinaryIO
{
private:
    FilePath &File;

public:
    BinaryIO(FilePath &File) : File(File) {}
    uint64_t getFileSize(const char *filePathToScan);
    void scanner();
    void WriteBinaryStandard(std::ofstream &outfile, FileDetails &details);
    
};

void scanFlow(FilePath &File);
void readerForCompression();
void readerForDecompression();

void appendMagicStatic(const char *outputFilePath);
bool fileIsExist(const char *outPutFilePath);

template <typename T>
void write_binary_le(std::ofstream &file, T value)
{
    file.write((const char *)&value, sizeof(T));
}

template <typename T>
T read_binary_le(std::ifstream &file)
{
    T value;
    file.read((char *)&value, sizeof(T));
    return value;
}

#endif
