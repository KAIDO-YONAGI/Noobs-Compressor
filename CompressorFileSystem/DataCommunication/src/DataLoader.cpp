#include "../include/DataLoader.h"

void DataLoader::done()
{
    if (inFile.is_open())
    {
        inFile.close();
    }
    loadIsDone = true;
    readCount = 0;
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
            throw std::runtime_error("reset()-Error:Failed to open inFile Path:" + EncodingUtils::pathToUtf8(inPath));
        loadIsDone = false;
    }
    else
        throw std::runtime_error("reset()-Error:inFile is still open, cannot reset to new path:" + EncodingUtils::pathToUtf8(inPath));
}
void DataLoader::resetByLastRead()
{
    Locator locator;
    locator.locateFromBegin(inFile, readCount);
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
    readCount += inFile.gcount();
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
