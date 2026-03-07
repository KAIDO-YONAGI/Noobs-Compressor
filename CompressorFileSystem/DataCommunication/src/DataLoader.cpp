#include "../include/DataLoader.h"
namespace fs = std::filesystem;
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
void DataLoader::reset(const fs::path inPath)
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
        data.resize(BUFFER_SIZE); // 确保缓冲区大小正确
        inFile.read(reinterpret_cast<char *>(data.data()), BUFFER_SIZE);
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
    readed += inFile.gcount();
}
void DataLoader::dataLoader(Y_flib::FileSize readSize, std::ifstream &loadFile, Y_flib::DataBlock &data)
{
    try
    {
        data.resize(BUFFER_SIZE); // 确保缓冲区大小正确
        loadFile.read(reinterpret_cast<char *>(data.data()), readSize);
        data.resize(loadFile.gcount()); // 缩小数组到实际读取的大小，避免后续处理时误读未初始化的部分
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Error-dataLaoder_decompression()");
    }
}
