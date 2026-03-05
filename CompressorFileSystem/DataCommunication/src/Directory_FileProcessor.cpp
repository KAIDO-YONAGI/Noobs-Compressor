// Directory_FileProcessor.cpp
#include "../include/Directory_FileProcessor.h"
namespace fs = std::filesystem;

void Directory_FileProcessor::directory_fileProcessor(const  std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot)
{

    fs::path oPath = fullOutPath;
    fs::path sPath;

    file.setOutPutFilePath(oPath);

    try
    {
        Y_flib::FileCount num = filePathToScan.size();

        // 预留回填偏移量的字节位置
        Y_flib::DirectoryOffsetSize tempOffset = 0; // 初始偏移量
        Y_flib::DirectoryOffsetSize offset = HEADER_SIZE;

        BIO->writeBlankSeparatedStandard();

        BIO->writeLogicalRoot(logicalRoot, num, tempOffset); // 写入逻辑根节点的子文件数目（默认创建一个根节点，用户可以选择是否命名）
        BIO->writeRoot(file, filePathToScan, tempOffset);  // 写入文件根目录

        for (Y_flib::FileCount i = 0; i < num; i++)
        {

            sPath = transfer.transPath(filePathToScan[i]);
            if (!fs::is_regular_file(sPath))
            {
                file.setFilePathToScan(sPath);
                BIO->binaryIO_Writer(file, directory_FileQueue, tempOffset, offset); // 添加当前目录到队列以启动整个BFS递推
            }
        }
        scanFlow(file, tempOffset, offset);
    }
    catch (const std::exception &e)
    {
        std::cerr << "directory_fileProcessor()-Error: " << e.what() << std::endl;
    }
}

void Directory_FileProcessor::scanFlow(FilePath &file, Y_flib::DirectoryOffsetSize &tempOffset, Y_flib::DirectoryOffsetSize &offset)
{

    BinaryIO_Writer BIO(outFile);

    while (!directory_FileQueue.empty())
    {
        Directory_FileDetails &details = (directory_FileQueue.front()).first;
        file.setFilePathToScan(details.getFullPath());

        BIO.binaryIO_Writer(file, directory_FileQueue, tempOffset, offset);

        directory_FileQueue.pop();
    }
}
