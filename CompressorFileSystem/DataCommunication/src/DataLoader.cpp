#include "../include/DataLoader.h"

void DataLoader::done()
{
    if (inFile.is_open())
    {
        inFile.close();
    }
    loadIsDone = true;
    data.clear();
}
void DataLoader::reset(const fs::path inPath)
{
    if (isDone())
    {
        std::ifstream newInFile(inPath, std::ios::binary);
        if (!newInFile)
            throw std::runtime_error("reset()-Error:Failed to open inFile Path:" + inPath.string());
        this->inFile = std::move(newInFile);
        loadIsDone = false;
    }
    else
        throw std::runtime_error("reset()-Error:inFile is still open, cannot reset to new path:" + inPath.string());
}
void DataLoader::dataLoader()
{
    try
    {

        data.resize(BUFFER_SIZE);
        inFile.read(reinterpret_cast<char *>((data.data())), BUFFER_SIZE);
        data.resize(inFile.gcount());
    }
    catch (const std::exception &e)
    {
        // std::cerr << e.what() << '\n';
        throw std::runtime_error("Error-dataLaoder_compression()");
    }

    if (inFile.gcount() == 0)
    {
        done();
    }
}
void DataLoader::dataLoader(FileSize_uint readSize, std::ifstream &decompressionFile)
{
    try
    {
        data.resize(readSize);
        decompressionFile.read(reinterpret_cast<char *>((data.data())), readSize);
        data.resize(decompressionFile.gcount());
    }
    catch (const std::exception &e)
    {
        // std::cerr << e.what() << '\n';
        throw std::runtime_error("Error-dataLaoder_decompression()");
    }
}
