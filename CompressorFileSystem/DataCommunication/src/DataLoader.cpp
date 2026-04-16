#include "../include/DataLoader.h"

void DataLoader::done()
{
    if (inFile.is_open())
    {
        inFile.close();
    }
    loadIsDone = true;
    readed = 0;
    data.clear();
}
void DataLoader::reset(const std::filesystem::path inPath)
{
    if (isDone())
    {
        if (inFile.is_open())
        {
            inFile.close();
        }
        inFile = std::ifstream(inPath, std::ios::binary);
        if (!inFile)
            throw std::runtime_error("reset()-Error:Failed to open inFile Path:" + inPath.string());
        loadIsDone = false;
    }
    else
        throw std::runtime_error("reset()-Error:inFile is still open, cannot reset to new path:" + inPath.string());
}
void DataLoader::resetByLastReaded()
{
    Locator locator;
    locator.locateFromBegin(inFile, readed);
}
void DataLoader::dataLoader()
{
    if (isDone())
        return;
    try
    {
        StandardsReader::readDataBlock(Y_flib::Constants::BUFFER_SIZE, inFile, data);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Error-dataLaoder_compression()");
    }

    if (inFile.gcount() == 0)
    {
        done();
    }
    readed += inFile.gcount();
}
void DataLoader::dataLoader(Y_flib::FileSize readSize, std::ifstream &loadFile, Y_flib::DataBlock &data)
{
    try
    {
        StandardsReader::readDataBlock(readSize, loadFile, data);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Error-dataLaoder_decompression()");
    }
}
