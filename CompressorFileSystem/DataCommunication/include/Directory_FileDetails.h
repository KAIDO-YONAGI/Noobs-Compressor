// Directory_FileDetails.h
#pragma once

#include "FileLibrary.h"

/* Directory_FileDetails - 文件/目录的元数据容器
 *
 * 功能:
 *   存储单个文件或目录的基本属性
 *   包括名称、大小、类型和完整路径
 *   用于目录遍历和二进制序列化过程中传递信息
 */
class Directory_FileDetails
{
private:
    std::string name;
    FileNameSize_uint sizeOfName;
    FileSize_uint fileSize;
    bool isFile;
    fs::path fullPath;

public:
    /* 构造函数，初始化文件/目录元数据 */
    Directory_FileDetails(std::string name, FileNameSize_uint sizeOfName, FileSize_uint fileSize, bool isFile, fs::path fullPath)
        : name(std::move(name)), sizeOfName(sizeOfName), fileSize(fileSize), isFile(isFile), fullPath(fullPath) {}

    /* 获取文件/目录名称 */
    const std::string getName() { return name; }

    /* 获取完整路径 */
    const fs::path getFullPath() { return fullPath; }

    /* 获取名称长度 */
    const FileNameSize_uint getSizeOfName() { return sizeOfName; }

    /* 获取文件大小 */
    const FileSize_uint getFileSize() { return fileSize; }

    /* 检查是否为文件（false表示目录） */
    const bool getIsFile() { return isFile; }

    /* 更新文件大小 */
    void setFileSize(FileSize_uint fileSize) { this->fileSize = fileSize; }
};

/* FilePath - 输入输出路径管理器
 *
 * 功能:
 *   管理源路径和目标输出路径
 *   为文件扫描和处理流程提供路径信息
 */
class FilePath
{
private:
    fs::path outPutFilePath;
    fs::path filePathToScan;

public:
    /* 默认构造函数 */
    FilePath() {}

    /* 设置输出文件路径 */
    void setOutPutFilePath(const fs::path outPutFilePath)
    {
        this->outPutFilePath = outPutFilePath;
    }

    /* 用于重新设置要扫描的源路径，复用对象 */
    void setFilePathToScan(const fs::path filePathToScan)
    {
        this->filePathToScan = filePathToScan;
    }

    /* 获取输出文件路径 */
    const fs::path getOutPutFilePath() { return outPutFilePath; }

    /* 获取要扫描的源路径 */
    const fs::path getFilePathToScan() { return filePathToScan; }
};
