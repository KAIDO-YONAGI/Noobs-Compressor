// EntryProcessor.cpp
#include "../include/EntryProcessor.h"

namespace Y_flib
{

    void EntryProcessor::entryProcessor(const std::vector<std::string> &filePathToScan, const std::filesystem::path &fullOutPath, const std::string &logicalRoot)
    {

        std::filesystem::path oPath = fullOutPath;
        std::filesystem::path sPath;

        file.setOutputFilePath(oPath);

        try
        {
            Y_flib::FileCount num = filePathToScan.size();

            // 目录区写入游标：默认初值即"紧跟文件头的第一个空分割标准槽位"（原 tempOffset/offset 手工初始化）
            Y_flib::BinaryStandardWriter::DirectoryScanCursor cursor;

            binaryStandardWriter->writeBlankSeparatedStandard();

            binaryStandardWriter->writeLogicalRoot(logicalRoot, num, cursor); // 写入逻辑根节点的子文件数目（默认创建一个根节点，用户可以选择是否命名）
            binaryStandardWriter->writeRoot(file, filePathToScan, cursor);    // 写入文件根目录

            for (Y_flib::FileCount i = 0; i < num; i++)
            {

                sPath = EncodingUtils::pathFromUtf8(filePathToScan[i]);
                if (!std::filesystem::is_regular_file(sPath))
                {
                    file.setFilePathToScan(sPath);
                    binaryStandardWriter->binaryStandardWriter(file, entryQueue, cursor); // 添加当前目录到队列以启动整个BFS递推
                }
            }
            flowScanner(file, cursor);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("EntryProcessor encountered an error: ") + e.what());
        }
    }

    void EntryProcessor::flowScanner(FilePath &file, Y_flib::BinaryStandardWriter::DirectoryScanCursor &cursor)
    {

        BinaryStandardWriter binaryStandardWriter(outFile);

        while (!entryQueue.empty())
        {
            EntryDetails &details = (entryQueue.front()).first;
            file.setFilePathToScan(details.getFullPath());

            binaryStandardWriter.binaryStandardWriter(file, entryQueue, cursor);

            entryQueue.pop();
        }
    }
} // namespace Y_flib
