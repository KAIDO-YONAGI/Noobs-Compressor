#include "../include/EntryParser.h"
namespace fs = std::filesystem;

void EntryParser::checkBounds(Y_flib::DirectoryOffsetSize blockPosition, Y_flib::FileNameSize requiredSize) const
{
    if (blockPosition + requiredSize > buffer.size())
    {
        throw std::out_of_range(
            "BinaryStandardLoader: Buffer overflow (blockPosition=" +
            std::to_string(blockPosition) + ", required=" +
            std::to_string(requiredSize) + ", buffer size=" +
            std::to_string(buffer.size()) + ")");
    }
}

fs::path EntryParser::pathConnector(std::string &fileName)
{
    fs::path pathToProcess;
    if (!entryQueue.empty())
    {
        fs::path lastPath = entryQueue.front().first.getFullPath();
        pathToProcess = lastPath / fileName;
    }
    else
        throw("entryQueue is empty");
    return pathToProcess;
}

void EntryParser::fileParser(Y_flib::DirectoryOffsetSize &bufferPtr, bool isRoot)
{
    // 解析文件名偏移量
    std::string fileName;
    fs::path pathToProcess;
    Y_flib::FileNameSize fileNameSize = 0;
    Y_flib::FileSize compressedSize = 0;
    Y_flib::FileSize lastOffset = 0;

    // 解析信息，注意和下一步的顺序不能颠倒，否则会导致读写协议不对称，后续解析失败

    fileDetailsParser(fileNameSize, fileName, bufferPtr);
    // 解析文件原大小
    Y_flib::FileSize originSize = readDataFromReadedBlock<Y_flib::FileSize>(bufferPtr);

    if (parserMode == 1) // for compression
    {
        lastOffset = header.directoryOffset - (offset + tempOffset) + bufferPtr;
        bufferPtr += sizeof(Y_flib::FileSize); // skip compressedSize
    }
    else if (parserMode == 2) // for decompression
    {
        compressedSize = readDataFromReadedBlock<Y_flib::FileSize>(bufferPtr); // compressedSize
    }

    if (isRoot) // 如果在处理根目录下的第一级文件，就不进行路径拼接
        pathToProcess = tempPathForRootPaser;

    else
        pathToProcess = pathConnector(fileName);

    EntryDetails fileDetails(
        fileName,
        fileNameSize,
        originSize,
        true,
        pathToProcess);
    fileQueue.push({fileDetails, parserMode == 1 ? lastOffset : compressedSize});
}

void EntryParser::directoryParser(Y_flib::DirectoryOffsetSize &bufferPtr, bool isRoot)
{
    // 解析目录名偏移量
    // 解析目录名，后续拼接为绝对路径之后入队
    Y_flib::FileNameSize directoryNameSize = 0;
    std::string directoryName;
    fs::path pathToProcess;

    // 解析信息，注意和下一步的顺序不能颠倒，否则会导致读写协议不对称，后续解析失败
    fileDetailsParser(directoryNameSize, directoryName, bufferPtr);

    // 解析下级文件数量
    Y_flib::FileCount count = readDataFromReadedBlock<Y_flib::FileCount>(bufferPtr);

    if (isRoot) // 如果在处理根目录下的第一级文件，就不进行路径拼接
        pathToProcess = tempPathForRootPaser;

    else
        pathToProcess = pathConnector(directoryName);

    EntryDetails directoryDetails(directoryName, directoryNameSize, 0, false, pathToProcess);
    entryQueue.push({directoryDetails, count});
}

void EntryParser::rootParser(Y_flib::DirectoryOffsetSize &bufferPtr, const std::vector<std::string> &filePathToScan, Y_flib::FileCount &countOfChildDirectory, bool &noDirec)
{
    Y_flib::FileNameSize directoryNameSize = 0;
    std::string directoryName;
    // 解析逻辑根
    fileDetailsParser(directoryNameSize, directoryName, bufferPtr);
    // 解析下级文件数量
    Y_flib::FileCount count = readDataFromReadedBlock<Y_flib::FileCount>(bufferPtr);

    countOfChildDirectory = count;

    if (parserMode == 2) // 解压模式,把逻辑根写进队列
    {
        fs::path root = transfer.transPath(rootForDecompression);
        fs::path file = transfer.transPath(directoryName);
        fs::path fullPath = root / file;
        EntryDetails logicalRootDetails(directoryName, directoryNameSize, 0, false, fullPath);
        entryQueue.push({logicalRootDetails, count});
    }
    else if (parserMode == 1) // 压缩模式
    {
        for (const std::string &path : filePathToScan)
        {
            tempPathForRootPaser = transfer.transPath(path); // 用类成员变量暂存路径，供后续根目录下的文件和目录进行路径拼接

            const Y_flib::FlagType entryFlag = readDataFromReadedBlock<Y_flib::FlagType>(bufferPtr);

            switch (entryFlag)
            {
            case Y_flib::FlagType::File:
                fileParser(bufferPtr, true);
                break;
            case Y_flib::FlagType::Directory:
                directoryParser(bufferPtr, true);
                break;
            default:
                throw std::runtime_error("rootParser()-Error:Failed to read flag");
            }
        }
        if (entryQueue.empty())
            noDirec = true;
    }
}

void EntryParser::parser(Y_flib::DirectoryOffsetSize &bufferPtr, Y_flib::FileCount &countOfChildDirectory)
{
    if (tempOffset <= bufferPtr && tempOffset != 0)
        return;

    const Y_flib::FlagType entryFlag = readDataFromReadedBlock<Y_flib::FlagType>(bufferPtr);
    switch (entryFlag)
    {
    case Y_flib::FlagType::File:
    {
        fileParser(bufferPtr, false);
        countOfChildDirectory--;
        break;
    }
    case Y_flib::FlagType::Directory:
    {
        directoryParser(bufferPtr, false);
        countOfChildDirectory--;
        break;
    }
    case Y_flib::FlagType::LogicalRoot:
    {
        bool noDirec = false;
        rootParser(bufferPtr, filePathToScan, countOfChildDirectory, noDirec);
        if (!noDirec)
            countOfChildDirectory = entryQueue.front().second; // 启动递推
        break;
    }

    default:
    {
        throw std::runtime_error("parser()-Error:Failed to read flag");
    }
    }
}