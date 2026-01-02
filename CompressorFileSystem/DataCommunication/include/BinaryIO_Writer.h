#pragma once

#include "FileLibrary.h"
#include "ToolClasses.h"
#include "Directory_FileDetails.h"

/* BinaryIO_Writer - 二进制目录结构序列化写入器
 *
 * 功能:
 *   扫描本地文件系统并序列化为二进制目录结构
 *   分层处理目录和文件，生成标准化的目录块
 *   支持逻辑根节点和多文件任务处理
 *   写入分隔符标记用于区分不同数据块
 *
 * 公共接口:
 *   binaryIO_Writer(): 主写入函数，扫描并序列化目录结构
 *   writeRoot(): 写入根节点（filesystem自动忽略的节点）
 *   writeLogicalRoot(): 写入逻辑根节点，用于多文件任务
 *   writeBlankSeparatedStandard(): 写入分隔符标记
 */
class BinaryIO_Writter
{
private:
    Transfer transfer;
    NumsWriter numWriter;
    Locator locator;
    std::ofstream &outFile;

    /* 序列化单个目录及其子元素，写入目录标准格式 */
    void writeDirectoryStandard(Directory_FileDetails &details, FileCount_uint count, DirectoryOffsetSize_uint &tempOffset);

    /* 序列化单个文件的元数据，写入文件标准格式 */
    void writeFileStandard(Directory_FileDetails &details, DirectoryOffsetSize_uint &tempOffset);

    /* 写入分隔符标记分离多个目录块 */
    void writeSeparatedStandard(DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint offset);

    /* 分发当前路径上的目录/文件到相应的写入处理函数 */
    void writeStorageStandard(Directory_FileDetails &details, Directory_FIleQueueInterface &directoryQueue, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset);

    /* 处理符号链接的序列化写入 */
    void writeSymbolLinkStandard(Directory_FileDetails &details, DirectoryOffsetSize_uint &tempOffset);

    /* 统计指定目录下的文件总数（不递归） */
    FileCount_uint countFilesInDirectory(const fs::path &filePathToScan);

    /* 获取指定文件的大小 */
    FileSize_uint getFileSize(const fs::path &filePathToScan);

public:
    /* 构造函数，初始化写入器并关联输出文件流 */
    explicit BinaryIO_Writter(std::ofstream &outFile) : outFile(outFile) {};

    /* 写入逻辑根节点，用于处理多文件（目录）任务 */
    void writeLogicalRoot(const std::string &logicalRoot, const FileCount_uint count, DirectoryOffsetSize_uint &tempOffset);

    /* 写入根节点，处理filesystem自动忽略的根目录 */
    void writeRoot(FilePath &file, const std::vector<std::string> &filePathToScan, DirectoryOffsetSize_uint &tempOffset);

    /* 写入空白分隔符标记 */
    void writeBlankSeparatedStandard();

    /* 写入用于加密的空白分隔符标记 */
    void writeBlankSeparatedStandardForEncryption(std::fstream &File);

    /* 主扫描函数，递归扫描并序列化目录结构到二进制格式 */
    void binaryIO_Writer(FilePath &file, Directory_FIleQueueInterface &directoryQueue, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset);
};
