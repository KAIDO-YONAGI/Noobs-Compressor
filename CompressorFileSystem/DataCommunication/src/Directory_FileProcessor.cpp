// Directory_FileProcessor.cpp
#include "../include/Directory_FileProcessor.h"

void Directory_FileProcessor::directory_fileProcessor(const  std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot)
{

    fs::path oPath = fullOutPath;
    fs::path sPath;

    file.setOutPutFilePath(oPath);

    try
    {
        FileCount_uint length = filePathToScan.size();

        // 预留回填偏移量的字节位置
        DirectoryOffsetSize_uint tempOffset = 0; // 初始偏移量
        DirectoryOffsetSize_uint offset = HEADER_SIZE;

        BIO->writeBlankSeparatedStandard();

        BIO->writeLogicalRoot(logicalRoot, length, tempOffset); // 写入逻辑根节点的子文件数目（默认创建一个根节点，用户可以选择是否命名）
        BIO->writeRoot(file, filePathToScan, tempOffset);  // 写入文件根目录

        for (FileCount_uint i = 0; i < length; i++)
        {

            sPath = transfer.transPath(filePathToScan[i]);
            if (!fs::is_regular_file(sPath))
            {
                file.setFilePathToScan(sPath);
                BIO->binaryIO_Writer(file, directoryQueue, tempOffset, offset); // 添加当前目录到队列以启动整个BFS递推
            }
        }
        scanFlow(file, tempOffset, offset);
    }
    catch (const std::exception &e)
    {
        std::cerr << "directory_fileProcessor()-Error: " << e.what() << std::endl;
    }
}

void Directory_FileProcessor::scanFlow(FilePath &file, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset)
{

    BinaryIO_Writter BIO(outFile);

    while (!directoryQueue.Directory_FileQueue.empty())
    {
        Directory_FileDetails &details = (directoryQueue.Directory_FileQueue.front()).first;
        file.setFilePathToScan(details.getFullPath());

        BIO.binaryIO_Writer(file, directoryQueue, tempOffset, offset);

        directoryQueue.Directory_FileQueue.pop();
    }
}
