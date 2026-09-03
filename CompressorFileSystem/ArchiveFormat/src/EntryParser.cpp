#include "../include/EntryParser.h"

namespace Y_flib
{
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

    std::filesystem::path EntryParser::pathConnector(std::string &fileName)
    {
        std::filesystem::path pathToProcess;
        if (!entryQueue.empty())
        {
            std::filesystem::path lastPath = entryQueue.front().first.getFullPath();
            // fileName 是 UTF-8 编码，需要正确转换为 path
            // 在 Windows 上，直接从 UTF-8 字符串构造 path 可能会出错
            // 使用 PathTransfer 进行转换
            std::filesystem::path fileNamePath = EncodingUtils::pathFromUtf8(fileName);
            pathToProcess = lastPath / fileNamePath;
        }
        else
            throw("entryQueue is empty");
        return pathToProcess;
    }

    void EntryParser::fileParser(Y_flib::DirectoryOffsetSize &bufferPtr, bool isRoot)
    {
        // 解析文件名偏移量
        std::string fileName;
        std::filesystem::path pathToProcess;
        Y_flib::FileNameSize fileNameSize = 0;
        Y_flib::FileSize compressedSize = 0;
        Y_flib::SlotOffset lastOffset = 0; // 该文件"处理后大小"预留字段在归档中的绝对偏移

        // 解析信息，注意和下一步的顺序不能颠倒，否则会导致读写协议不对称，后续解析失败

        fileDetailsParser(fileNameSize, fileName, bufferPtr);
        // 解析文件原大小
        Y_flib::FileSize originSize = readDataFromReadBlock<Y_flib::FileSize>(bufferPtr);

        if (parserMode == 1) // for compression
        {
            lastOffset = cursor.entryFieldPos(header.directoryOffset, bufferPtr); // "处理后大小"预留槽的绝对偏移
            bufferPtr += sizeof(Y_flib::FileSize); // skip compressedSize
        }
        else if (parserMode == 2) // for decompression
        {
            compressedSize = readDataFromReadBlock<Y_flib::FileSize>(bufferPtr); // compressedSize
        }

        if (isRoot) // 如果在处理根目录下的第一级文件，就不进行路径拼接
            pathToProcess = tempPathForRootParser;

        else
            pathToProcess = pathConnector(fileName);

        EntryDetails fileDetails(
            fileName,
            fileNameSize,
            originSize,
            true,
            pathToProcess);

        if (parserMode == 1)
            fileQueue.push({fileDetails, lastOffset, 0}); // 压缩：记录预留槽偏移，供压缩完成后回填
        else
            fileQueue.push({fileDetails, 0, compressedSize}); // 解压：记录压缩后大小，供逐块读出
    }

    void EntryParser::directoryParser(Y_flib::DirectoryOffsetSize &bufferPtr, bool isRoot)
    {
        // 解析目录名偏移量
        // 解析目录名，后续拼接为绝对路径之后入队
        Y_flib::FileNameSize directoryNameSize = 0;
        std::string directoryName;
        std::filesystem::path pathToProcess;

        // 解析信息，注意和下一步的顺序不能颠倒，否则会导致读写协议不对称，后续解析失败
        fileDetailsParser(directoryNameSize, directoryName, bufferPtr);

        // 解析下级文件数量
        Y_flib::FileCount count = readDataFromReadBlock<Y_flib::FileCount>(bufferPtr);

        if (isRoot) // 如果在处理根目录下的第一级文件，就不进行路径拼接
            pathToProcess = tempPathForRootParser;

        else
            pathToProcess = pathConnector(directoryName);

        EntryDetails directoryDetails(directoryName, directoryNameSize, 0, false, pathToProcess);
        entryQueue.push({directoryDetails, count});
    }

    void EntryParser::linkParser(
        Y_flib::DirectoryOffsetSize &bufferPtr,
        bool isRoot,
        Y_flib::FlagType linkType)
    {
        if (header.version < 2)
        {
            throw std::runtime_error(
                "Legacy v1 symbolic-link metadata cannot be restored safely");
        }

        // 链接标准的两个长度字段连续存放，随后才是名称和目标正文。
        const Y_flib::FileNameSize nameSize =
            readDataFromReadBlock<Y_flib::FileNameSize>(bufferPtr);
        const Y_flib::FileNameSize targetSize =
            readDataFromReadBlock<Y_flib::FileNameSize>(bufferPtr);

        checkBounds(bufferPtr, nameSize);
        std::string name(
            buffer.data() + bufferPtr,
            buffer.data() + bufferPtr + nameSize);
        bufferPtr += nameSize;

        checkBounds(bufferPtr, targetSize);
        std::string target(
            buffer.data() + bufferPtr,
            buffer.data() + bufferPtr + targetSize);
        bufferPtr += targetSize;

        const std::filesystem::path linkPath =
            isRoot ? tempPathForRootParser : pathConnector(name);
        if (parserMode == 2)
        {
            linkQueue.push({
                linkType,
                linkPath,
                EncodingUtils::pathFromUtf8(target)});
        }
    }

    void EntryParser::rootParser(Y_flib::DirectoryOffsetSize &bufferPtr, const std::vector<std::string> &filePathToScan, Y_flib::FileCount &countOfChildDirectory, bool &noDirec)
    {
        Y_flib::FileNameSize directoryNameSize = 0;
        std::string directoryName;
        // 解析逻辑根
        fileDetailsParser(directoryNameSize, directoryName, bufferPtr);
        // 解析下级文件数量
        Y_flib::FileCount count = readDataFromReadBlock<Y_flib::FileCount>(bufferPtr);

        countOfChildDirectory = count;

        if (parserMode == 2) // 解压模式,把逻辑根写进队列
        {
            std::filesystem::path root = EncodingUtils::pathFromUtf8(rootForDecompression);
            std::filesystem::path file = EncodingUtils::pathFromUtf8(directoryName);
            std::filesystem::path fullPath = root / file;
            EntryDetails logicalRootDetails(directoryName, directoryNameSize, 0, false, fullPath);
            entryQueue.push({logicalRootDetails, count});
        }
        else if (parserMode == 1) // 压缩模式
        {
            for (const std::string &path : filePathToScan)
            {
                tempPathForRootParser = EncodingUtils::pathFromUtf8(path); // 用类成员变量暂存路径，供后续根目录下的文件和目录进行路径拼接

                const Y_flib::FlagType entryFlag = readDataFromReadBlock<Y_flib::FlagType>(bufferPtr);

                switch (entryFlag)
                {
                case Y_flib::FlagType::File:
                    fileParser(bufferPtr, true);
                    break;
                case Y_flib::FlagType::Directory:
                    directoryParser(bufferPtr, true);
                    break;
                case Y_flib::FlagType::SymbolicLinkFile:
                case Y_flib::FlagType::SymbolicLinkDirectory:
                case Y_flib::FlagType::Junction:
                    // 根链接只消费目录元数据，不加入普通文件数据队列。
                    linkParser(bufferPtr, true, entryFlag);
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
        if (cursor.blockEndReached(bufferPtr))
            return;

        const Y_flib::FlagType entryFlag = readDataFromReadBlock<Y_flib::FlagType>(bufferPtr);
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
        case Y_flib::FlagType::SymbolicLinkFile:
        case Y_flib::FlagType::SymbolicLinkDirectory:
        case Y_flib::FlagType::Junction:
        {
            linkParser(bufferPtr, false, entryFlag);
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
} // namespace Y_flib
