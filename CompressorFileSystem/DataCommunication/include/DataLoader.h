// DataLoader.h
#pragma once

#include "FileLibrary.h"

class DataLoader
{
private:
    DataBlock buffer = DataBlock(BUFFER_SIZE);

    FileSize_uint fileSize = 0;
    std::ifstream inFile;
    bool loadIsDone= false;
    void done();

public:
    const DataBlock &getBlock()
    {
        // if (!isDone())
        return buffer;
    }
    bool isDone()
    {
        return loadIsDone;
    }
    void reset(fs::path inPath)
    {
        if (isDone())
        {
            std::ifstream newInFile(inPath, std::ios::binary);
            if (!newInFile)
                throw std::runtime_error("reset()-Error:Failed to open inFile Path:" + inPath.string());
            this->inFile = std::move(newInFile);
            loadIsDone=false;
        }
        else
            throw std::runtime_error("reset()-Error:inFile is still open, cannot reset to new path:" + inPath.string());
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
    void setFileSize(FileSize_uint newSize)
    {
        fileSize = newSize;
    }
    void dataLoader(FileSize_uint readSize);
};