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
 *   调用BinaryStandardWriter进行二进制序列化
 *
 * 公共接口:
 *   entryProcessor(): 主入口函数，启动文件系统扫描和处理
 */
namespace Y_flib
{
    class EntryProcessor
    {
    private:
        EntryQueue entryQueue;
        // 写入器与处理器生命周期一致，直接按值保存，避免手动 new/delete 和复制双重释放。
        BinaryStandardWriter binaryStandardWriter;

        /* BFS扫描目录并处理每个文件/子目录，游标位置算术收口在 DirectoryScanCursor 内 */
        void flowScanner(Y_flib::BinaryStandardWriter::DirectoryScanCursor &cursor);

    public:
        /* 将归档输出流直接交给值成员写入器。 */
        explicit EntryProcessor(std::ofstream &outFile) : binaryStandardWriter(outFile) {}

        /* 主处理函数，执行指定路径的文件系统扫描和二进制序列化 */
        void entryProcessor(
            const std::vector<std::string> &filePathToScan,
            const std::string &logicalRoot);
    };
} // namespace Y_flib
