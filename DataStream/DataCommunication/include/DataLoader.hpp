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
        buffer.clear();
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
                throw std::runtime_error("reset()-Error:Failed to open inFile Path:"+inPath.string());
            this->inFile = std::move(newInFile);
        }
        else
            return;
    }
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
    void dataLoader()
    {
        try
        {
            buffer.resize(BUFFER_SIZE);
            inFile.read(buffer.data(), BUFFER_SIZE);
            buffer.resize(inFile.gcount());//避免空数据问题
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