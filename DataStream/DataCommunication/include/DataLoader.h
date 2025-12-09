// DataLoader.h
#pragma once

#include "../include/FileLibrary.h"

class DataLoader
{
private:
    std::ifstream inFile;
    void done()
    {
        if (inFile.is_open())
        {
            inFile.close();
        }
    }

public:
    bool isDone()
    {
        return !inFile.is_open();
    }

    DataLoader(fs::path &inPath)
    {
        std::ifstream inFile(inPath, std::ios::binary);
        if (!inFile)
            throw std::runtime_error("DataLoader()-Error:Failed to open inFile");
        this->inFile = std::move(inFile);
    };
    ~DataLoader()
    {
        if (inFile.is_open())
        {
            inFile.close();
        }
    }
    std::vector<char> dataLoader();
};