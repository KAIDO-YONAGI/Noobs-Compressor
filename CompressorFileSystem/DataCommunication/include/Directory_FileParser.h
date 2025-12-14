// Paser.hpp
#pragma once

#include "FileLibrary.h"
#include "Directory_FileDetails.h"
#include "ToolClasses.h"
class Directory_FileParser
{
private:
    Directory_FileQueue &directoryQueue;
    Directory_FileQueue &fileQueue;
    std::vector<std::string> &filePathToScan;
    DataBlock &buffer;
    Transfer transfer;
    const Header &header;
    const DirectoryOffsetSize_uint &offset;
    const DirectoryOffsetSize_uint &tempOffset;
    void checkBounds(DirectoryOffsetSize_uint pos, FileNameSize_uint equiredSize) const;
    size_t parserMode = 0; // 0：默认模式、1：压缩模式、2：解压模式

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
            checkBounds(bufferPtr, static_cast<FileNameSize_uint>(fileNameSize));
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
    fs::path pathConnector(std::string &fileName);

    void fileParser(DirectoryOffsetSize_uint &bufferPtr);
    void directoryParser(DirectoryOffsetSize_uint &bufferPtr);
    void rootParser(DirectoryOffsetSize_uint &bufferPtr,const std::vector<std::string> &filePathToScan,FileCount_uint &countOfKidDirectory,bool &noDirec);

public:
    std::string rootForDecompression;
    void setRootForDecompression(std::string rootForDecompression)
    {
        this->rootForDecompression = rootForDecompression;
    }
    Directory_FileParser(DataBlock &buffer, Directory_FileQueue &directoryQueue,
                         Directory_FileQueue &fileQueue, const Header &header,
                         const DirectoryOffsetSize_uint &offset,
                         const DirectoryOffsetSize_uint &tempOffset,
                         std::vector<std::string> &filePathToScan)
        : buffer(buffer), directoryQueue(directoryQueue),
          fileQueue(fileQueue),
          header(header),
          offset(offset), tempOffset(tempOffset),filePathToScan(filePathToScan)
    {
        parserMode=((!filePathToScan.empty())?1:2);//非空表示压缩
    }
    void parser(DirectoryOffsetSize_uint &bufferPtr, FileCount_uint &countOfKidDirectory);
};