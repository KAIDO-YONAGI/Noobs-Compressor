// DataExporter.hpp
#pragma once

#include "FileLibrary.h"
#include "Directory_FileProcessor.h"
#include "ToolClasses.h"
class DataExporter
{
private:
    std::fstream outFile;
    std::ofstream tempFilePtr;
    Locator locator;
    FileSize_uint processedFileSize = 0;

    void thisBlockIsDone(DirectoryOffsetSize_uint dataSize);

public:
    DataExporter(const fs::path &outPath)
    {
        std::fstream outFile(outPath, std::ios::binary | std::ios::out | std::ios::in);
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
    void thisFileIsDone(FileSize_uint offsetToFill);
    void exportDataToFile_Encryption(const std::vector<char> &data);
};