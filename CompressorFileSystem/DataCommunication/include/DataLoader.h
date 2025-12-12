// DataLoader.h
#pragma once

#include "FileLibrary.h"

class DataLoader
{
private:
    std::vector<char> buffer = std::vector<char>(BUFFER_SIZE);

    std::ifstream inFile;
    void done();

public:
    const std::vector<char>& getBlock(){
        return buffer;
    }
    bool isDone()
    {
        return !inFile.is_open();
    }
    void reset(fs::path inPath);
    DataLoader(const fs::path &inPath)
    {
        std::ifstream inFile(inPath, std::ios::binary);
        if (!inFile)
            throw std::runtime_error("DataLoader()-Error:Failed to open inFile Path:"+inPath.string());
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
};