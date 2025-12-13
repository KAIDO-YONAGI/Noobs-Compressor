#include "../include/DataLoader.h"

void DataLoader::done()
{
    if (inFile.is_open())
    {
        inFile.close();
    }
    buffer.clear();
}

void DataLoader::reset(fs::path inPath)
{
    if (isDone())
    {
        std::ifstream newInFile(inPath, std::ios::binary);
        if (!newInFile)
            throw std::runtime_error("reset()-Error:Failed to open inFile Path:"+inPath.string());
        this->inFile = std::move(newInFile);
    }
    else
        throw std::runtime_error("reset()-Error:inFile is still open, cannot reset to new path:"+inPath.string());
}

void DataLoader::dataLoader()
{
    try
    {
        buffer.resize(BUFFER_SIZE);
        inFile.read(buffer.data(), BUFFER_SIZE);
        buffer.resize(inFile.gcount());
    }
    catch (const std::exception &e)
    {
        // std::cerr << e.what() << '\n';
        throw std::runtime_error("Error-dataLaoder()");
    }

    if (inFile.gcount() == 0)
    {
        done();
    }
}