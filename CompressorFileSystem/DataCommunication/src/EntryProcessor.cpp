// EntryProcessor.cpp
#include "../include/EntryProcessor.h"

namespace Y_flib
{

void EntryProcessor::entryProcessor(const  std::vector<std::string> &filePathToScan, const std::filesystem::path &fullOutPath, const std::string &logicalRoot)
{

    std::filesystem::path oPath = fullOutPath;
    std::filesystem::path sPath;

    file.setOutputFilePath(oPath);

    try
    {
        Y_flib::FileCount num = filePathToScan.size();

        // 预留回填偏移量的字节位置
        Y_flib::DirectoryOffsetSize tempOffset = 0; // 初始偏移量
        Y_flib::DirectoryOffsetSize offset = Y_flib::Constants::HEADER_SIZE;

        binaryStandardWriter->writeBlankSeparatedStandard();

        binaryStandardWriter->writeLogicalRoot(logicalRoot, num, tempOffset); // 写入逻辑根节点的子文件数目（默认创建一个根节点，用户可以选择是否命名）
        binaryStandardWriter->writeRoot(file, filePathToScan, tempOffset);  // 写入文件根目录

        for (Y_flib::FileCount i = 0; i < num; i++)
        {

            sPath = EncodingUtils::pathFromUtf8(filePathToScan[i]);
            if (!std::filesystem::is_regular_file(sPath))
            {
                file.setFilePathToScan(sPath);
                binaryStandardWriter->binaryStandardWriter(file, entryQueue, tempOffset, offset); // 添加当前目录到队列以启动整个BFS递推
            }
        }
        flowScanner(file, tempOffset, offset);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("EntryProcessor encountered an error: ") + e.what());
    }
}

void EntryProcessor::flowScanner(FilePath &file, Y_flib::DirectoryOffsetSize &tempOffset, Y_flib::DirectoryOffsetSize &offset)
{

    BinaryStandardWriter binaryStandardWriter(outFile);

    while (!entryQueue.empty())
    {
        EntryDetails &details = (entryQueue.front()).first;
        file.setFilePathToScan(details.getFullPath());

        binaryStandardWriter.binaryStandardWriter(file, entryQueue, tempOffset, offset);

        entryQueue.pop();
    }
}
} // namespace Y_flib
