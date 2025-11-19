// DirectoryReader.h
#ifndef DIRECTORYREADER
#define DIRECTORYREADER

#include "../include/FileLibrary.h"
#include "../include/FileQueue.h"
#include "../include/FileDetails.h"
#include "../include/Transfer.h"

class BinaryIO_Reader;//类的前向声明
class FilePath; 
class DirectoryReader;

class DirectoryReader
{
public:
    DirectoryReader() = default;
    void writeLogicalRoot(FilePath &File,std::string &logicalRoot, FileCount_Int count);
    void writeRoot(FilePath &File, std::vector<std::string> &filePathToScan);
    void scanFlow(FilePath &File);
    void appendMagicStatic(fs::path &outputFilePath);
    void headerReader(std::vector<std::string>& filePathToScan1,std::string &outPutFilePath1,std::string &logicalRoot);
    FileCount_Int countFilesInDirectory(fs::path &filePathToScan);
};

class FilePath
{
private:
    fs::path outPutFilePath;
    fs::path filePathToScan;

public:
    FilePath(const FilePath &) = delete;
    FilePath() {}
    void setOutPutFilePath(fs::path &outPutFilePath){
        this->outPutFilePath=outPutFilePath;
    }
    void setFilePathToScan(fs::path &filePathToScan)
    {
        this->filePathToScan = filePathToScan;
    }

    fs::path &getOutPutFilePath() { return outPutFilePath; }
    fs::path &getFilePathToScan() { return filePathToScan; }
};

class BinaryIO_Reader
{
public:
    BinaryIO_Reader() = default;
    FileSize_Int getFileSize(fs::path &filePathToScan);
    void scanner(FilePath &File, QueueInterface &queue);
    void writeBinaryStandard(std::ofstream &outFile, FileDetails &details, QueueInterface &queue);
    void writeHeaderStandard(std::ofstream &outFile, FileDetails &details, FileCount_Int count);
    void writeFileStandard(std::ofstream &outFile, FileDetails &details);
};

template <typename T>
void write_binary_le(std::ofstream &file, T value)
{
    file.write(reinterpret_cast<char *>(&value), sizeof(T)); //不做类型检查，直接进行类型转换
}

#endif
