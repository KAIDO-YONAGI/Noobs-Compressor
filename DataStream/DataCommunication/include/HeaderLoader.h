// HeaderLoader.h
#ifndef HEADERLOADER_H
#define HEADERLOADER_H

#include "../include/FileLibrary.h"
#include "../include/FileDetails.h"
#include "../include/ToolClasses.hpp"

class FilePath_Loader
{
private:
    fs::path outPutFilePath;
    fs::path filePathToScan;
};

class BinaryIO_Loader
{
private:
    std::vector<unsigned char> &buffer;
    std::ifstream &inFile;

    // 检查 buffer 是否足够读取指定大小
    inline void checkBounds(DirectoryOffsetSize_uint pos, size_t requiredSize) const
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
    void fileNameSizeParser(
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
                "fileNameSizeParser failed at offset " +
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

public:
    BinaryIO_Loader(std::vector<unsigned char> &buffer, std::ifstream &inFile)
        : buffer(buffer), inFile(inFile)
    {
        if (!inFile)
            throw std::runtime_error("Invalid input file stream");
    }
    void headerLoader(); // 主逻辑函数

#pragma pack(1) // 禁用填充，紧密读取
    struct Header
    {
        SizeOfMagicNum_uint magicNum_1 = 0;
        CompressStrategy_uint strategy = 0;
        CompressorVersion_uint version = 0;
        HeaderOffsetSize_uint headerOffset = 0;
        DirectoryOffsetSize_uint directoryOffset = 0;
        SizeOfMagicNum_uint magicNum_2 = 0;
    };
};

#endif
