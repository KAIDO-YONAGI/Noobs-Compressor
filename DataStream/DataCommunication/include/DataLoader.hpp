// DataLoader.hpp
#pragma once

#include "../include/FileLibrary.h"

class DataLoader
{
private:
    std::vector<char> buffer = std::vector<char>(BUFFER_SIZE);

    std::ifstream inFile;
    void done()
    {
        if (inFile.is_open())
        {
            inFile.close();
        }
    }

public:
    const std::vector<char>& getBlock(){
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
                throw std::runtime_error("reset()-Error:Failed to open inFile");
            this->inFile = std::move(newInFile);
        }
        else
            throw std::runtime_error("reset()-Error:inFile is already open");
    }
    DataLoader(const fs::path &inPath)
    {
        std::ifstream inFile(inPath, std::ios::binary);
        if (!inFile)
            throw std::runtime_error("DataLoader()-Error:Failed to open inFile"+inPath.string());
        this->inFile = std::move(inFile);
    };
    ~DataLoader()
    {
        if (inFile.is_open())
        {
            inFile.close();
        }
    }
    void dataLoader()
    {
        try
        {
            inFile.read(buffer.data(), BUFFER_SIZE);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }

        if (inFile.gcount() == 0)
        {
            done();
        }
    }
};