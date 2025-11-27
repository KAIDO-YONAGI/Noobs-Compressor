// Directory_FileProcessor.h
#ifndef DIRECTORY_FILEPROCESSOR_H
#define DIRECTORY_FILEPROCESSOR_H

#include "../include/FileLibrary.h"
#include "../include/FileDetails.h"
#include "../include/ToolClasses.hpp"

class BinaryIO_Reader; // 类的前向声明
class Directory_FileProcessor;

class Directory_FileProcessor
{
    /*
    writeLogicalRoot()写入逻辑根节点，用于处理多文件（目录）任务，具体情况可见调试代码
    writeRoot()写入根节点（因为filesystem自动忽略了根节点）
    scanFlow()按BFS（层序遍历）扫描指定目录下所有文件的函数。包含了队列逻辑，用于处理scanner()的循环扫描到的目录
    directory_fileProcessor()当前类的主逻辑函数。参数const std::vector<std::string> &filePathToScan，用于处理多文件（目录）任务
    */
public:
    Directory_FileProcessor() = default;
    void writeLogicalRoot(FilePath &file, const std::string &logicalRoot, const FileCount_uint count, std::ofstream &outFile, DirectoryOffsetSize_uint &tempOffset);
    void writeRoot(FilePath &file, const std::vector<std::string> &filePathToScan, std::ofstream &outFile, DirectoryOffsetSize_uint &tempOffset);
    void scanFlow(FilePath &file, std::ofstream &outFile,DirectoryOffsetSize_uint &tempOffset,DirectoryOffsetSize_uint &offset);
    void directory_fileProcessor(const std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot, std::ofstream &outFile);
    FileCount_uint countFilesInDirectory(const fs::path &filePathToScan);
};

class BinaryIO_Reader // 接触二进制文件及其处理的相关IO的函数的封装
{
    /*
    scanner()扫描指定路径（单层）的文件及其子文件夹的函数
    writeStorageStandard()分发当前处理的路径上的目录/文件到：目录标准写入/文件标准写入
    writeHeaderStandard()目录标准写入
    writeFileStandard()文件标准写入
    */
public:
    BinaryIO_Reader() = default;
    FileSize_uint getFileSize(const fs::path &filePathToScan,std::ofstream &outFile);
    void scanner(FilePath &file, QueueInterface &queue, std::ofstream &outFile,DirectoryOffsetSize_uint &tempOffset,DirectoryOffsetSize_uint &offset);
    void writeStorageStandard(std::ofstream &outFile, FileDetails &details, QueueInterface &queue,FilePath &file,DirectoryOffsetSize_uint &tempOffset,DirectoryOffsetSize_uint &offset);

    void writeHeaderStandard(std::ofstream &outFile, FileDetails &details, FileCount_uint count, DirectoryOffsetSize_uint &tempOffset);
    void writeFileStandard(std::ofstream &outFile, FileDetails &details, DirectoryOffsetSize_uint &tempOffset);
    void writeSeparatedStandard(std::ofstream &outFile, FilePath &file,DirectoryOffsetSize_uint &tempOffset,DirectoryOffsetSize_uint offset);

    template <typename T>
    void writeBinary(std::ofstream &outFile, T value)
    {
        outFile.write(reinterpret_cast<char *>(&value), // 不做类型检查，直接进行类型转换
                      sizeof(T));
    }
};

#endif
