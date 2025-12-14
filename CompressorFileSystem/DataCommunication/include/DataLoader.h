// DataLoader.h
#pragma once

#include "FileLibrary.h"

class DataLoader
{
private:
    DataBlock buffer = DataBlock(BUFFER_SIZE);

    FileSize_uint fileSize;
    std::ifstream inFile;
    void done();

public:
    const DataBlock &getBlock()
    {
        // if (!isDone())
        return buffer;
    }
    bool isDone()
    {
        return !inFile.is_open();
    }
    void reset(fs::path inPath)
    {
        if (isDone())
        {
            std::ifstream newInFile(inPath, std::ios::binary);
            if (!newInFile)
                throw std::runtime_error("reset()-Error:Failed to open inFile Path:" + inPath.string());
            this->inFile = std::move(newInFile);
        }
        else
            throw std::runtime_error("reset()-Error:inFile is still open, cannot reset to new path:" + inPath.string());
    }
    void setFileSize(FileSize_uint newSize)
    {
        fileSize = newSize;
    }
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
    void dataLoader();
    void dataLoader(FileSize_uint readSize);
};