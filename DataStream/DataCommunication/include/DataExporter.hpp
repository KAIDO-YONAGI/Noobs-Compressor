// DataExporter.hpp
#pragma once

#include "../include/FileLibrary.h"
#include "Directory_FileProcessor.h"
#include "ToolClasses.hpp"
class DataExporter
{
private:
    std::fstream outFile;
    std::ofstream tempFilePtr;
    Locator locator;
    FileSize_uint processedFileSize;
    void thisFileIsDone(FileSize_uint offsetToFill)
    {
        outFile.seekp(offsetToFill, std::ios::beg);
        NumsWriter numWriter(tempFilePtr);
        numWriter.writeBinaryNums(processedFileSize,outFile);
        outFile.seekp(0, std::ios::end);
    }
    void thisBlockIsDone(DirectoryOffsetSize_uint dataSize)
    {
        outFile.seekp(outFile.tellp()- static_cast<std::streamoff>(dataSize+sizeof(DirectoryOffsetSize_uint)), std::ios::beg);
        NumsWriter numWriter(tempFilePtr);
        numWriter.writeBinaryNums(dataSize,outFile);
        outFile.seekp(0, std::ios::end);
    }

public:
    DataExporter(const fs::path &outPath)
    {
        std::fstream outFile(outPath, std::ios::binary | std::ios::out|std::ios::in);
        if (!outFile)
        {
            throw std::runtime_error("DataExporter()-Error:Failed to open outFile");
        }
        this->outFile = std::move(outFile);
    };
    ~DataExporter()
    {
        if (outFile.is_open())
        {
            outFile.close();
        }
    }
    void exportDataToFile_Encryption(const std::vector<char> &data, FileSize_uint offsetToFill)
    {
        if (!outFile)
        {
            throw std::runtime_error("exportDataToFile()-Error:Failed to open outFile");
        }
        BinaryIO_Writter processor(std::move(tempFilePtr));//此处只传入不使用(使用禁止)

        // TODO:此处需要回填偏移量
        outFile.seekp(0, std::ios::end);
        size_t blockSize = data.size();
        processor.writeBlankSeparatedStandardForEncryption(outFile);

        outFile.write(data.data(), blockSize);
        processedFileSize += blockSize;

        thisBlockIsDone(blockSize);

        if (offsetToFill != 0)
        {
            thisFileIsDone(offsetToFill);
        }
    }
};