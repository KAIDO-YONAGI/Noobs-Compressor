// Directory_FileProcessor.h
#pragma once

#include "FileLibrary.h"
#include "Directory_FileDetails.h"
#include "ToolClasses.h"
#include "BinaryIO_Writer.h"

/* Directory_FileProcessor - 目录文件处理和扫描器
 *
 * 功能:
 *   按BFS（层序遍历）扫描文件系统并处理每个目录
 *   支持多文件和多目录任务的统一处理
 *   管理目录队列用于层序遍历
 *   调用BinaryIO_Writer进行二进制序列化
 *
 * 公共接口:
 *   directory_fileProcessor(): 主入口函数，启动文件系统扫描和处理
 */
class Directory_FileProcessor
{
private:
    Transfer transfer;
    std::ofstream &outFile;
    Directory_FIleQueueInterface directoryQueue;
    FilePath file; // 创建各个工具类的对象
    BinaryIO_Writter *BIO;
    NumsWriter numWriter;

    /* BFS扫描目录并处理每个文件/子目录，维护偏移量 */
    void scanFlow(FilePath &file, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset);

public:
    /* 构造函数，初始化处理器并创建二进制写入器 */
    Directory_FileProcessor(std::ofstream &outFile) : outFile(outFile)
    {
        BIO = new BinaryIO_Writter(outFile);
    };

    /* 析构函数，释放二进制写入器资源 */
    ~Directory_FileProcessor()
    {
        delete BIO;
    };

    /* 主处理函数，执行指定路径的文件系统扫描和二进制序列化 */
    void directory_fileProcessor(const  std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot);
};