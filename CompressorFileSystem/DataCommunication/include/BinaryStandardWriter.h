#pragma once

#include "FileLibrary.h"
#include "ToolClasses.h"
#include "EntryDetails.h"

/* BinaryStandardWriter - 二进制目录结构序列化写入器
 *
 * 功能:
 *   扫描本地文件系统并序列化为二进制目录结构
 *   分层处理目录和文件，生成标准化的目录块
 *   支持逻辑根节点和多文件任务处理
 *   写入分隔符标记用于区分不同数据块
 *
 * 公共接口:
 *   binaryStandardWriter(): 主写入函数，扫描并序列化目录结构
 *   writeRoot(): 写入根节点（filesystem自动忽略的节点）
 *   writeLogicalRoot(): 写入逻辑根节点，用于多文件任务
 *   writeBlankSeparatedStandard(): 写入分隔符标记
 */
class BinaryStandardWriter
{
private:
    PathTransfer transfer;
    StandardWriter standardWriter;
    Locator locator;
    std::ofstream &outFile;

    /* 序列化单个目录及其子元素，写入目录标准格式 */
    void writeDirectoryStandard(EntryDetails &details, Y_flib::FileCount count, Y_flib::DirectoryOffsetSize &tempOffset);

    /* 序列化单个文件的元数据，写入文件标准格式 */
    void writeFileStandard(EntryDetails &details, Y_flib::DirectoryOffsetSize &tempOffset);

    /* 写入分隔符标记分离多个目录块 */
    void writeSeparatedStandard(Y_flib::DirectoryOffsetSize &tempOffset, Y_flib::DirectoryOffsetSize offset);

    /* 分发当前路径上的目录/文件到相应的写入处理函数 */
    void writeStorageStandard(EntryDetails &details, EntryQueue &directory_FileQueue, Y_flib::DirectoryOffsetSize &tempOffset, Y_flib::DirectoryOffsetSize &offset);

    /* 处理符号链接的序列化写入 */
    void writeSymbolLinkStandard(EntryDetails &details, Y_flib::DirectoryOffsetSize &tempOffset);

    /* 统计指定目录下的文件总数（不递归） */
    Y_flib::FileCount countFilesInDirectory(const std::filesystem::path &filePathToScan);

    /* 获取指定文件的大小 */
    Y_flib::FileSize getFileSize(const std::filesystem::path &filePathToScan);

public:
    /* 构造函数，初始化写入器并关联输出文件流 */
    BinaryStandardWriter(std::ofstream &outFile) : outFile(outFile) {};

    /* 写入逻辑根节点，用于处理多文件（目录）任务 */
    void writeLogicalRoot(const std::string &logicalRoot, const Y_flib::FileCount count, Y_flib::DirectoryOffsetSize &tempOffset);

    /* 写入根节点，处理filesystem自动忽略的根目录 */
    void writeRoot(FilePath &file, const std::vector<std::string> &filePathToScan, Y_flib::DirectoryOffsetSize &tempOffset);

    /* 写入空白分隔符标记 */
    void writeBlankSeparatedStandard();

    /* 写入用于加密的空白分隔符标记 */
    void writeBlankSeparatedStandardForEncryption(std::fstream &File);

    /* 主扫描函数，递归扫描并序列化目录结构到二进制格式 */
    void binaryStandardWriter(FilePath &file, EntryQueue &directory_FileQueue, Y_flib::DirectoryOffsetSize &tempOffset, Y_flib::DirectoryOffsetSize &offset);
};
