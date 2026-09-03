// EntryProcessor.cpp
#include "../include/EntryProcessor.h"
#include "../../Commons/include/FileSystemUtils.h"

namespace Y_flib
{

    void EntryProcessor::entryProcessor(
        const std::vector<std::string> &filePathToScan,
        const std::string &logicalRoot)
    {
        try
        {
            const Y_flib::FileCount num = filePathToScan.size();

            // 目录区写入游标：默认初值即"紧跟文件头的第一个空分割标准槽位"（原 tempOffset/offset 手工初始化）
            Y_flib::BinaryStandardWriter::DirectoryScanCursor cursor;

            binaryStandardWriter.writeBlankSeparatedStandard();

            binaryStandardWriter.writeLogicalRoot(logicalRoot, num, cursor); // 写入逻辑根节点的子文件数目（默认创建一个根节点，用户可以选择是否命名）
            binaryStandardWriter.writeRoot(filePathToScan, cursor);          // 写入文件根目录

            for (const std::string &path : filePathToScan)
            {
                const std::filesystem::path sPath = EncodingUtils::pathFromUtf8(path);
                const FileSystemEntryInfo rootInfo = FileSystemUtils::queryEntry(sPath);
                if (rootInfo.isDirectory)
                {
                    binaryStandardWriter.binaryStandardWriter(sPath, entryQueue, cursor); // 添加当前目录到队列以启动整个BFS递推
                }
            }
            flowScanner(cursor);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("EntryProcessor encountered an error: ") + e.what());
        }
    }

    void EntryProcessor::flowScanner(Y_flib::BinaryStandardWriter::DirectoryScanCursor &cursor)
    {
        while (!entryQueue.empty())
        {
            const std::filesystem::path directoryPath = entryQueue.front().first.getFullPath();
            binaryStandardWriter.binaryStandardWriter(directoryPath, entryQueue, cursor);

            entryQueue.pop();
        }
    }
} // namespace Y_flib
