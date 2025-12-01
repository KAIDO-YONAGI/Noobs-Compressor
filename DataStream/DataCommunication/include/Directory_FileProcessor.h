// Directory_FileProcessor.h
#pragma once

#include "../include/FileLibrary.h"
#include "../include/FileDetails.h"
#include "../include/ToolClasses.hpp"

class Directory_FileProcessor
{
    /*
    scanFlow()按BFS（层序遍历）扫描指定目录下所有文件的函数。包含了队列逻辑，用于处理binaryIO_Reader()的循环扫描到的目录
    directory_fileProcessor()主函数。参数const std::vector<std::string> &filePathToScan，用于处理多文件（目录）任务
    */
private:
    Transfer transfer;
    std::ofstream &outFile;
    QueueInterface queue;
    void scanFlow(FilePath &file, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset);

public:
    explicit Directory_FileProcessor(std::ofstream &outFile) : outFile(outFile) {};
    void directory_fileProcessor(const std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot);
};

class BinaryIO_Reader // 接触二进制文件及其处理的相关IO的函数的封装
{
    /*
    binaryIO_Reader()主函数。扫描指定路径（单层）的文件及其子文件夹的函数
    writeStorageStandard()分发当前处理的路径上的目录/文件到：目录标准写入/文件标准写入
    writeDirectoryStandard()目录标准写入
    writeFileStandard()文件标准写入
    writeLogicalRoot()写入逻辑根节点，用于处理多文件（目录）任务，具体情况可见调试代码
    writeRoot()写入根节点（因为filesystem自动忽略了根节点）
    */
private:
    Transfer transfer;
    std::ofstream &outFile;
    
    void writeDirectoryStandard(FileDetails &details, FileCount_uint count, DirectoryOffsetSize_uint &tempOffset);
    void writeFileStandard(FileDetails &details, DirectoryOffsetSize_uint &tempOffset);
    void writeSeparatedStandard(DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint offset);
    void writeStorageStandard(FileDetails &details, QueueInterface &queue, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset);
    FileCount_uint countFilesInDirectory(const fs::path &filePathToScan);
    FileSize_uint getFileSize(const fs::path &filePathToScan);

public:
    explicit BinaryIO_Reader(std::ofstream &outFile) : outFile(outFile) {};
    void writeLogicalRoot(const std::string &logicalRoot, const FileCount_uint count, DirectoryOffsetSize_uint &tempOffset);
    void writeRoot(FilePath &file, const std::vector<std::string> &filePathToScan, DirectoryOffsetSize_uint &tempOffset);
    void makeSeparatedStandard(std::ofstream &outFile);

    void binaryIO_Reader(FilePath &file, QueueInterface &queue, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset);
};

