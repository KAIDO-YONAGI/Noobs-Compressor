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

public:
    DataExporter(const fs::path &outPath)
    {
        std::ofstream outFile(outPath, std::ios::binary|std::ios::app);
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
    void exportDataToFile_Encryption(const std::vector<char> &data)
    {
        if (!outFile)
        {
            throw std::runtime_error("exportDataToFile()-Error:Failed to open outFile");
        }
        BinaryIO_Writter processor(outFile);

        processor.writeBlankSeparatedStandardForEncryption(outFile);
        outFile.write(data.data(), data.size());
    }
};