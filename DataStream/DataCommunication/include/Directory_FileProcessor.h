// Directory_FileProcessor.h
#ifndef DIRECTORY_FILEPROCESSOR
#define DIRECTORY_FILEPROCESSOR

#include "../include/FileLibrary.h"
#include "../include/FileDetails.h"
#include "../include/ToolClasses.hpp"

class BinaryIO_Reader; //类的前向声明
class Directory_FileProcessor;

class Directory_FileProcessor
{
public:
    Directory_FileProcessor() = default;
    void writeLogicalRoot(FilePath &File, const std::string &logicalRoot,const FileCount_uint count,std::ofstream &outFile);
    void writeRoot(FilePath &File, std::vector<std::string> &filePathToScan,std::ofstream &outFile);
    void scanFlow(FilePath &File,std::ofstream &outFile);
    void directory_fileProcessor(std::vector<std::string> &filePathToScan, const std::string &outPutFilePath, const std::string &logicalRoot,std::ofstream &outFile);
    FileCount_uint countFilesInDirectory(const fs::path &filePathToScan);
};

class BinaryIO_Reader
{
public:
    BinaryIO_Reader() = default;
    FileSize_uint getFileSize(fs::path &filePathToScan);
    void scanner(FilePath &File, QueueInterface &queue,std::ofstream &outFile);
    void writeBinaryStandard(std::ofstream &outFile, FileDetails &details, QueueInterface &queue);
    void writeHeaderStandard(std::ofstream &outFile, FileDetails &details, FileCount_uint count);
    void writeFileStandard(std::ofstream &outFile, FileDetails &details);
};

template <typename T>
void write_binary_le(std::ofstream &outFile, T value)
{
    outFile.write(reinterpret_cast<char *>(&value), sizeof(T)); //不做类型检查，直接进行类型转换
}

#endif
