#include "../include/Directory_FileParser.h"

void Directory_FileParser::checkBounds(DirectoryOffsetSize_uint pos, FileNameSize_uint requiredSize) const
{
    if (pos + requiredSize > buffer.size())
    {
        throw std::out_of_range(
            "BinaryIO_Loader: Buffer overflow (pos=" +
            std::to_string(pos) + ", required=" +
            std::to_string(requiredSize) + ", buffer size=" +
            std::to_string(buffer.size()) + ")");
    }
}

fs::path Directory_FileParser::pathConnector(std::string &fileName)
{
    fs::path pathToProcess;
    if (!directoryQueue.empty())
    {
        fs::path lastPath = directoryQueue.front().first.getFullPath();
        pathToProcess = lastPath / fileName;
    }
    else
        throw("directoryQueue is empty");
    return pathToProcess;
}

void Directory_FileParser::fileParser(DirectoryOffsetSize_uint &bufferPtr)
{
    // 解析文件名偏移量
    FileNameSize_uint fileNameSize = 0;
    std::string fileName;
    fs::path pathToProcess;
    fileName_fileSizeParser(fileNameSize, fileName, bufferPtr);

    // 解析文件原大小
    FileSize_uint originSize = numsParser<FileSize_uint>(bufferPtr);
    FileSize_uint compressedSize;
    if (parserMode == 1) // for compression
    {
        compressedSize = header.directoryOffset - (offset + tempOffset) + bufferPtr;
        bufferPtr += sizeof(FileSize_uint); // skip compressedSize
    }
    else if (parserMode == 2) // for decompression
    {
        compressedSize = numsParser<FileSize_uint>(bufferPtr);
    }

    pathToProcess = pathConnector(fileName);

    Directory_FileDetails fileDetails(
        fileName,
        fileNameSize,
        originSize,
        true,
        pathToProcess);
    fileQueue.push({fileDetails, compressedSize});
}

void Directory_FileParser::directoryParser(DirectoryOffsetSize_uint &bufferPtr)
{
    // 解析目录名偏移量
    // 解析目录名，后续拼接为绝对路径之后入队
    FileNameSize_uint directoryNameSize = 0;
    std::string directoryName;
    fileName_fileSizeParser(directoryNameSize, directoryName, bufferPtr);

    // 解析下级文件数量
    FileCount_uint count = numsParser<FileCount_uint>(bufferPtr);

    fs::path pathToProcess = pathConnector(directoryName);

    Directory_FileDetails directoryDetails(directoryName, directoryNameSize, 0, false, pathToProcess);
    directoryQueue.push({directoryDetails, count});
}

void Directory_FileParser::rootParser(DirectoryOffsetSize_uint &bufferPtr, std::vector<std::string> &filePathToScan)
{
    FileNameSize_uint directoryNameSize = 0;
    std::string directoryName;
    // 解析逻辑根
    fileName_fileSizeParser(directoryNameSize, directoryName, bufferPtr);
    // 解析下级文件数量
    FileCount_uint count = numsParser<FileCount_uint>(bufferPtr);
    if (parserMode == 2) // 解压模式,把逻辑根写进队列
    {
        fs::path root = transfer.transPath(rootForDecompression);
        fs::path file = transfer.transPath(directoryName);
        fs::path fullPath = root / file;
        Directory_FileDetails logicalRootDetails(directoryName, directoryNameSize, 0, false, fullPath);
        directoryQueue.push({logicalRootDetails, count});
    }
    else if (parserMode == 1)
    {
        for (const std::string &path : filePathToScan)
        {
            fs::path fullPath = transfer.transPath(path);
            const char D_F_flag = numsParser<char>(bufferPtr);

            if (D_F_flag == FILE_FLAG)
            {
                FileNameSize_uint fileNameSize = 0;
                std::string fileName;
                fileName_fileSizeParser(fileNameSize, fileName, bufferPtr);
                // 解析文件原大小
                FileSize_uint originSize = numsParser<FileSize_uint>(bufferPtr);
                // 记录等会需要回填的位置
                FileSize_uint compressedSize = header.directoryOffset - (offset + tempOffset) + bufferPtr;
                bufferPtr += sizeof(FileSize_uint);
                Directory_FileDetails fileDetails(
                    fileName,
                    fileNameSize,
                    originSize,
                    true,
                    fullPath);
                fileQueue.push({fileDetails, compressedSize});
            }
            else if (D_F_flag == DIRECTORY_FLAG)
            {
                FileNameSize_uint directoryNameSize = 0;
                std::string directoryName;
                fileName_fileSizeParser(directoryNameSize, directoryName, bufferPtr);
                // 解析下级文件数量
                FileCount_uint count = numsParser<FileCount_uint>(bufferPtr);

                Directory_FileDetails directoryDetails(directoryName, directoryNameSize, 0, false, fullPath);
                directoryQueue.push({directoryDetails, count});
            }
        }
    }
}

void Directory_FileParser::parser(DirectoryOffsetSize_uint &bufferPtr, FileCount_uint &countOfKidDirectory)
{
    if (tempOffset <= bufferPtr && tempOffset != 0)
        return;

    const char D_F_flag = numsParser<char>(bufferPtr);
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
        rootParser(bufferPtr, filePathToScan);
        countOfKidDirectory = directoryQueue.front().second; // 启动递推
        break;
    }

    default:
    {
        throw std::runtime_error("parser()-Error:Failed to read flag");
    }
    }
}