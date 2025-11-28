// Directory_FileProcessor.h
#ifndef DIRECTORY_FILEPROCESSOR_H
#define DIRECTORY_FILEPROCESSOR_H

#include "../include/FileLibrary.h"
#include "../include/FileDetails.h"
#include "../include/ToolClasses.hpp"

class BinaryIO_Reader // 接触二进制文件及其处理的相关IO的函数的封装
{
    private:
    std::ofstream &outFile;
    /*
    scanner()扫描指定路径（单层）的文件及其子文件夹的函数
    writeStorageStandard()分发当前处理的路径上的目录/文件到：目录标准写入/文件标准写入
    writeDirectoryStandard()目录标准写入
    writeFileStandard()文件标准写入
    */
public:
    BinaryIO_Reader() = default;
    FileSize_uint getFileSize(const fs::path &filePathToScan, std::ofstream &outFile);
    void scanner(FilePath &file, QueueInterface &queue, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset);
    void writeStorageStandard( FileDetails &details, QueueInterface &queue, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset);

    void writeDirectoryStandard(FileDetails &details, FileCount_uint count, DirectoryOffsetSize_uint &tempOffset);
    void writeFileStandard(FileDetails &details, DirectoryOffsetSize_uint &tempOffset);
    void writeSeparatedStandard(DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint offset);

    template <typename T>
    void writeBinaryNums(std::ofstream &outFile, T value)
    {
        // 编译时检查
        static_assert(std::is_trivially_copyable_v<T>,
                      "Cannot write non-trivially-copyable type");
        static_assert(!std::is_pointer_v<T>,
                      "Cannot safely write raw pointers");
        static_assert(!std::is_polymorphic_v<T>,
                      "Cannot safely write polymorphic types");
        outFile.write(reinterpret_cast<char *>(&value), // 不做类型检查，直接进行类型转换
                      sizeof(T));
    }
};

class Directory_FileProcessor
{
private:
    BinaryIO_Reader BIO;
    Transfer transfer;
    std::ofstream &outFile;
    /*
    writeLogicalRoot()写入逻辑根节点，用于处理多文件（目录）任务，具体情况可见调试代码
    writeRoot()写入根节点（因为filesystem自动忽略了根节点）
    scanFlow()按BFS（层序遍历）扫描指定目录下所有文件的函数。包含了队列逻辑，用于处理scanner()的循环扫描到的目录
    directory_fileProcessor()当前类的主逻辑函数。参数const std::vector<std::string> &filePathToScan，用于处理多文件（目录）任务
    */
public:
    Directory_FileProcessor(std::ofstream &outFile):outFile(outFile){};
    void writeLogicalRoot(const std::string &logicalRoot, const FileCount_uint count,DirectoryOffsetSize_uint &tempOffset);
    void writeRoot(FilePath &file, const std::vector<std::string> &filePathToScan, DirectoryOffsetSize_uint &tempOffset);
    void scanFlow(FilePath &file, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset);
    void directory_fileProcessor(const std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot);
    FileCount_uint countFilesInDirectory(const fs::path &filePathToScan);
};

#endif
