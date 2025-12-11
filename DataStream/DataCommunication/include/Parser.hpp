// Paser.hpp
#pragma once

#include "../include/FileLibrary.h"
#include "../include/Directory_FileDetails.h"
#include "../include/ToolClasses.hpp"
class Parser
{
private:
    Directory_FileQueue &directoryQueue;
    Directory_FileQueue &fileQueue;
    std::vector<unsigned char> &buffer;
    Transfer transfer;
    const Header &header;
    const DirectoryOffsetSize_uint &offset;
    const DirectoryOffsetSize_uint &tempOffset;

    void checkBounds(DirectoryOffsetSize_uint pos, size_t requiredSize) const
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

    template <typename T>
    void fileName_fileSizeParser(
        T &fileNameSize,
        std::string &fileName,
        DirectoryOffsetSize_uint &bufferPtr)
    {
        try
        {
            // 1. 读取文件名长度
            checkBounds(bufferPtr, sizeof(T));
            memcpy(&fileNameSize, buffer.data() + bufferPtr, sizeof(T));
            bufferPtr += sizeof(T);

            // 2. 读取字符串内容,安全构造 std::string,防止未初始化的越界报错
            checkBounds(bufferPtr, static_cast<size_t>(fileNameSize));
            fileName.assign(
                buffer.data() + bufferPtr,
                buffer.data() + bufferPtr + fileNameSize);
            bufferPtr += fileNameSize;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(
                "fileName_fileSizeParser failed at offset " +
                std::to_string(bufferPtr) + ": " + e.what());
        }
    }

    template <typename T>
    T numsParser(DirectoryOffsetSize_uint &bufferPtr)
    {
        try
        {
            T num;
            checkBounds(bufferPtr, sizeof(T));
            memcpy(&num, buffer.data() + bufferPtr, sizeof(T));
            bufferPtr += sizeof(T);

            return num;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(
                "numsParser failed at offset " +
                std::to_string(bufferPtr) + ": " + e.what());
        }
    }
    fs::path pathConnector(std::string &fileName)
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
    void fileParser(DirectoryOffsetSize_uint &bufferPtr)
    {
        // 解析文件名偏移量
        // 解析文件名，后续拼接为绝对路径之后交给数据读取类读取数据块
        FileNameSize_uint fileNameSize = 0;
        std::string fileName;
        fileName_fileSizeParser(fileNameSize, fileName, bufferPtr);

        // 解析文件原大小
        FileSize_uint originSize = numsParser<FileSize_uint>(bufferPtr);

        // 记录等会需要回填的位置
        FileSize_uint compressedSizeOffset = header.directoryOffset - (offset + tempOffset) + bufferPtr;
        bufferPtr += sizeof(FileSize_uint);

        fs::path pathToProcess = pathConnector(fileName);

        // std::cout << pathToProcess << "  ";
        if (fileName == "index.d.ts")
            int a = 1;
        if (bufferPtr > 8191)
            int a = 1;
        Directory_FileDetails fileDetails(
            fileName,
            fileNameSize,
            originSize,
            true,
            pathToProcess);
        fileQueue.push({fileDetails, compressedSizeOffset});
    }
    void directoryParser(DirectoryOffsetSize_uint &bufferPtr)
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

    void rootParser(DirectoryOffsetSize_uint &bufferPtr, std::vector<std::string> &filePathToScan)
    {

        FileNameSize_uint directoryNameSize = 0;
        std::string directoryName;
        fileName_fileSizeParser(directoryNameSize, directoryName, bufferPtr);
        // 解析下级文件数量
        FileCount_uint count = numsParser<FileCount_uint>(bufferPtr);

        if (count != filePathToScan.size()) // 检验数量
            throw std::runtime_error("rootParser()-Error:Failed to match RootDirectory nums");

        for (std::string &path : filePathToScan)
        {
            fs::path fullPath = transfer.transPath(path);
            if (fs::is_regular_file(fullPath))
            {
                bufferPtr += sizeof(SizeOfFlag_uint); // 步过文件标
                fileParser(bufferPtr);                // 遇到文件直接处理，调用fileParser
            }
            else if (fs::is_directory(fullPath))
            {
                bufferPtr += sizeof(SizeOfFlag_uint); // 步过文件标
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

public:
    Parser(std::vector<unsigned char> &buffer, Directory_FileQueue &directoryQueue, Directory_FileQueue &fileQueue, const Header &header, const DirectoryOffsetSize_uint &offset, const DirectoryOffsetSize_uint &tempOffset)
        : buffer(buffer), directoryQueue(directoryQueue), fileQueue(fileQueue), header(header), offset(offset), tempOffset(tempOffset) {}
    void parser(DirectoryOffsetSize_uint &bufferPtr, std::vector<std::string> &filePathToScan, FileCount_uint &countOfKidDirectory)
    {

        if (tempOffset <= bufferPtr && tempOffset != 0)
            return;

        unsigned char D_F_flag = numsParser<unsigned char>(bufferPtr);
        switch (D_F_flag)
        {
        case '1':
        {
            fileParser(bufferPtr);
            countOfKidDirectory--;

            break;
            // countOfD_F++;
        }
        case '0':
        {
            directoryParser(bufferPtr);
            countOfKidDirectory--;
            break;
        }
        case '3': // 逻辑根本身不入队，入队接下来的几个根目录，并且处理文件
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
        return;
    }
};