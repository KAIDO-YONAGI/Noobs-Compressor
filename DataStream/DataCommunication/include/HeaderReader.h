// HeaderReader.h
#ifndef HEADERREADER
#define HEADERREADER

#include <filesystem> //编译时需要强制链接为静态库

#include <fstream>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <queue>
#include <cassert>

namespace fs = std::filesystem;

class FilePath; //类的前向声明
class Locator;
class FileDetails;
class FileQueue;
class BinaryIO;

class HeaderReader
{

public:
    HeaderReader() = default;
    void writeRoot(FilePath &File);
    void scanFlow(FilePath &File);
    void appendMagicStatic(const fs::path &outputFilePath);
};

class Locator
{
public:
    void relativeLocator(std::ofstream &File, int offset);
    void relativeLocator(std::ifstream &File, int offset);
    void relativeLocator(std::fstream &File, int offset) = delete;
};

class FilePath
{
private:
    fs::path outPutFilePath;
    fs::path filePathToScan;

public:
    FilePath(const fs::path &outPutFilePath, const fs::path &filePathToScan)
        : outPutFilePath(outPutFilePath), filePathToScan(filePathToScan) {}

    void setFilePathToScan(const fs::path &filePathToScan)
    {
        this->filePathToScan = filePathToScan;
    }

    const fs::path &getOutPutFilePath() const { return outPutFilePath; }
    const fs::path &getFilePathToScan() const { return filePathToScan; }
};
class FileDetails
{
private:
    std::string name;
    uint8_t sizeOfName;
    uint64_t fileSize;
    bool isFile;
    fs::path fullPath;

public:
    FileDetails() = default;
    const std::string &getName() const { return name; }
    const fs::path &getFullPath() const { return fullPath; }
    uint8_t getSizeOfName() const { return sizeOfName; }
    uint64_t getFileSize() const { return fileSize; }
    bool getIsFile() const { return isFile; }

    FileDetails(std::string name, uint8_t sizeOfName, uint64_t fileSize, bool isFile, const fs::path &fullPath)
        : name(std::move(name)), sizeOfName(sizeOfName), fileSize(fileSize), isFile(isFile), fullPath(fullPath) {}
};
class FileQueue
{
public:
    std::queue<std::pair<FileDetails, int>> fileQueue;
};

class BinaryIO
{
public:
    BinaryIO() = default;
    uint64_t getFileSize(const fs::path &filePathToScan);
    void scanner(FilePath &File, FileQueue &queue);
    void writeBinaryStandard(std::ofstream &outfile, FileDetails &details, FileQueue &queue);
    void writeHeaderStandard(std::ofstream &outfile, FileDetails &details, uint32_t count);
    void writeFileStandard(std::ofstream &outfile, FileDetails &details);
};

void readerForCompression();
void readerForDecompression();
bool fileIsExist(const fs::path &outPutFilePath);
uint32_t countFilesInDirectory(const fs::path &filePathToScan);

template <typename T>
void write_binary_le(std::ofstream &file, T value)
{
    file.write(reinterpret_cast<const char *>(&value), sizeof(T)); //不做类型检查，直接进行类型转换
}

template <typename T>
T read_binary_le(std::ifstream &file)
{
    T value;
    file.read(reinterpret_cast<char *>(&value), sizeof(T));
    return value;
}

#endif
