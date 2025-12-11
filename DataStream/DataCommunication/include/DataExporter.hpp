// DataExporter.hpp
#pragma once

#include "../include/FileLibrary.h"
#include "Directory_FileProcessor.h"
#include "ToolClasses.hpp"
class DataExporter
{
private:
    std::ofstream outFile;
    Locator locator;
    FileSize_uint processedFileSize;
    void thisFileIsDone(FileSize_uint offsetToFill)
    {
        // locator.offsetLocator(outFile, offsetToFill);
        // NumsWriter numWriter(outFile);
        // numWriter.writeBinaryNums(processedFileSize);
        // outFile.seekp(0, std::ios::end);
    }
    void thisBlockIsDone(DirectoryOffsetSize_uint dataSize)
    {
        // locator.offsetLocator(outFile, outFile.tellp() - static_cast<std::streamoff>(sizeof(DirectoryOffsetSize_uint)));
        // NumsWriter numWriter(outFile);
        // numWriter.writeBinaryNums(dataSize);
        // outFile.seekp(0, std::ios::end);
    }

public:
    DataExporter(const fs::path &outPath)
    {
        std::ofstream outFile(outPath, std::ios::binary |std::ios::out| std::ios::app);
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
        BinaryIO_Writter processor(outFile);

        processor.writeBlankSeparatedStandardForEncryption(outFile);
        // TODO:此处需要回填偏移量

        size_t blockSize = data.size();
        outFile.seekp(0, std::ios::end);
        outFile.write(data.data(), static_cast<std::streamsize>(blockSize));
        processedFileSize += blockSize;
        thisBlockIsDone(blockSize);

        if(offsetToFill!=0){
            thisFileIsDone(offsetToFill);
        }
    }
};