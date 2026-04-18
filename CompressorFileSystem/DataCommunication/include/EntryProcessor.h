// EntryProcessor.h
#pragma once

#include "FileLibrary.h"
#include "EntryDetails.h"
#include "ToolClasses.h"
#include "BinaryStandardWriter.h"

/* EntryProcessor - 目录文件处理和扫描器
 *
 * 功能:
 *   按BFS（层序遍历）扫描文件系统并处理每个目录
 *   支持多文件和多目录任务的统一处理
 *   管理目录队列用于层序遍历
 *   调用BinaryIStandardWriter行二进制序列化
 *
 * 公共接口:
 *   entryProcessor(): 主入口函数，启动文件系统扫描和处理
 */
namespace Y_flib
{
class EntryProcessor
{
private:
    std::ofstream &outFile;
    EntryQueue entryQueue;
    FilePath file; // 创建各个工具类的对象
    BinaryStandardWriter *binaryStandardWriter;
    StandardsWriter standardWriter;

    /* BFS扫描目录并处理每个文件/子目录，维护偏移量 */
    void flowScanner(FilePath &file, Y_flib::DirectoryOffsetSize &tempOffset, Y_flib::DirectoryOffsetSize &offset);

public:
    /* 构造函数，初始化处理器并创建二进制写入器 */
    EntryProcessor(std::ofstream &outFile) : outFile(outFile)
    {
        binaryStandardWriter = new BinaryStandardWriter(outFile);
    };

    /* 析构函数，释放二进制写入器资源 */
    ~EntryProcessor()
    {
        delete binaryStandardWriter;
    };

    /* 主处理函数，执行指定路径的文件系统扫描和二进制序列化 */
    void entryProcessor(const  std::vector<std::string> &filePathToScan, const std::filesystem::path &fullOutPath, const std::string &logicalRoot);
};
} // namespace Y_flib
