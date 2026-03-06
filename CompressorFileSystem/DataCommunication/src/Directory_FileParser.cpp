#include "../include/Directory_FileParser.h"
namespace fs = std::filesystem;

void Directory_FileParser::checkBounds(Y_flib::DirectoryOffsetSize pos, Y_flib::FileNameSize requiredSize) const
{
    if (pos + requiredSize > buffer.size())
    {
        throw std::out_of_range(
            "BinaryStandardLoader: Buffer overflow (pos=" +
            std::to_string(pos) + ", required=" +
            std::to_string(requiredSize) + ", buffer size=" +
            std::to_string(buffer.size()) + ")");
    }
}

fs::path Directory_FileParser::pathConnector(std::string &fileName)
{
    fs::path pathToProcess;
    if (!directory_FileQueue.empty())
    {
        fs::path lastPath = directory_FileQueue.front().first.getFullPath();
        pathToProcess = lastPath / fileName;
    }
    else
        throw("directory_FileQueue is empty");
    return pathToProcess;
}

void Directory_FileParser::fileParser(Y_flib::DirectoryOffsetSize &bufferPtr)
{
    // 解析文件名偏移量
    Y_flib::FileNameSize fileNameSize = 0;
    std::string fileName;
    fs::path pathToProcess;
    fileName_fileSizeParser(fileNameSize, fileName, bufferPtr);

    // 解析文件原大小
    Y_flib::FileSize originSize = numsParser<Y_flib::FileSize>(bufferPtr);
    Y_flib::FileSize compressedSize_or_Offset;
    if (parserMode == 1) // for compression
    {
        compressedSize_or_Offset = header.directoryOffset - (offset + tempOffset) + bufferPtr;
        bufferPtr += sizeof(Y_flib::FileSize); // skip compressedSize
    }
    else if (parserMode == 2) // for decompression
    {
        compressedSize_or_Offset = numsParser<Y_flib::FileSize>(bufferPtr); // compressedSize
    }

    pathToProcess = pathConnector(fileName);

    Directory_FileDetails fileDetails(
        fileName,
        fileNameSize,
        originSize,
        true,
        pathToProcess);
    fileQueue.push({fileDetails, compressedSize_or_Offset});
}

void Directory_FileParser::directoryParser(Y_flib::DirectoryOffsetSize &bufferPtr)
{
    // 解析目录名偏移量
    // 解析目录名，后续拼接为绝对路径之后入队
    Y_flib::FileNameSize directoryNameSize = 0;
    std::string directoryName;
    fileName_fileSizeParser(directoryNameSize, directoryName, bufferPtr);

    // 解析下级文件数量
    Y_flib::FileCount count = numsParser<Y_flib::FileCount>(bufferPtr);

    fs::path pathToProcess = pathConnector(directoryName);

    Directory_FileDetails directoryDetails(directoryName, directoryNameSize, 0, false, pathToProcess);
    directory_FileQueue.push({directoryDetails, count});
}

void Directory_FileParser::rootParser(Y_flib::DirectoryOffsetSize &bufferPtr, const std::vector<std::string> &filePathToScan, Y_flib::FileCount &countOfKidDirectory, bool &noDirec)
{
    Y_flib::FileNameSize directoryNameSize = 0;
    std::string directoryName;
    // 解析逻辑根
    fileName_fileSizeParser(directoryNameSize, directoryName, bufferPtr);
    // 解析下级文件数量
    Y_flib::FileCount count = numsParser<Y_flib::FileCount>(bufferPtr);

    countOfKidDirectory = count;

    if (parserMode == 2) // 解压模式,把逻辑根写进队列
    {
        fs::path root = transfer.transPath(rootForDecompression);
        fs::path file = transfer.transPath(directoryName);
        fs::path fullPath = root / file;
        Directory_FileDetails logicalRootDetails(directoryName, directoryNameSize, 0, false, fullPath);
        directory_FileQueue.push({logicalRootDetails, count});
    }
    else if (parserMode == 1)
    {
        for (const std::string &path : filePathToScan)
        {
            fs::path fullPath = transfer.transPath(path);
            const char D_F_flag = numsParser<char>(bufferPtr);

            if (D_F_flag == FILE_FLAG)
            {
                Y_flib::FileNameSize fileNameSize = 0;
                std::string fileName;
                fileName_fileSizeParser(fileNameSize, fileName, bufferPtr);
                // 解析文件原大小
                Y_flib::FileSize originSize = numsParser<Y_flib::FileSize>(bufferPtr);
                // 记录等会需要回填的位置
                Y_flib::FileSize compressedSize_or_Offset = header.directoryOffset - (offset + tempOffset) + bufferPtr;
                bufferPtr += sizeof(Y_flib::FileSize);
                Directory_FileDetails fileDetails(
                    fileName,
                    fileNameSize,
                    originSize,
                    true,
                    fullPath);
                fileQueue.push({fileDetails, compressedSize_or_Offset});
            }
            else if (D_F_flag == DIRECTORY_FLAG)
            {
                Y_flib::FileNameSize directoryNameSize = 0;
                std::string directoryName;
                fileName_fileSizeParser(directoryNameSize, directoryName, bufferPtr);
                // 解析下级文件数量
                Y_flib::FileCount count = numsParser<Y_flib::FileCount>(bufferPtr);

                Directory_FileDetails directoryDetails(directoryName, directoryNameSize, 0, false, fullPath);
                directory_FileQueue.push({directoryDetails, count});
            }
        }
        if (directory_FileQueue.empty())
            noDirec = true;
    }
}

void Directory_FileParser::parser(Y_flib::DirectoryOffsetSize &bufferPtr, Y_flib::FileCount &countOfKidDirectory)
{
    if (tempOffset <= bufferPtr && tempOffset != 0)
        return;

    const unsigned char D_F_flag = numsParser<unsigned char>(bufferPtr);
    switch (D_F_flag)
    {
    case FILE_FLAG:
    {
        fileParser(bufferPtr);
        countOfKidDirectory--;
        break;
        // countOfD_F++;
    }
    case DIRECTORY_FLAG:
    {
        directoryParser(bufferPtr);
        countOfKidDirectory--;
        break;
    }
    case LOGICAL_ROOT_FLAG:
    {
        bool noDirec = false;
        rootParser(bufferPtr, filePathToScan, countOfKidDirectory, noDirec);
        if (!noDirec)
            countOfKidDirectory = directory_FileQueue.front().second; // 启动递推
        break;
    }

    default:
    {
        throw std::runtime_error("parser()-Error:Failed to read flag");
    }
    }
}