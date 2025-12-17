// DataLoader.h
#pragma once

#include "FileLibrary.h"

class DataLoader
{
private:
    DataBlock data = DataBlock(BUFFER_SIZE);

    FileSize_uint fileSize = 0;
    std::ifstream inFile;
    bool loadIsDone = false;
    FileSize_uint readed=0;

    void done();

public:
    const DataBlock &getBlock() { return data; }
    bool isDone() { return loadIsDone; }
    void reset(const fs::path inPath);

    void dataLoader();
    void dataLoader(FileSize_uint readSize, std::ifstream &decompressionFile);
    void setFileSize(FileSize_uint newSize) { fileSize = newSize; }
    void resetByLastReaded();

    DataLoader() {}
    DataLoader(const fs::path &inPath)
    {
        std::ifstream inFile(inPath, std::ios::binary);
        if (!inFile)
            throw std::runtime_error("DataLoader()-Error:Failed to open inFile Path:" + inPath.string());
        this->inFile = std::move(inFile);
    };

    ~DataLoader()
    {
        if (inFile.is_open())
        {
            inFile.close();
        }
    }
};