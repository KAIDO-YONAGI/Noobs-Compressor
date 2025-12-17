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
void DataLoader::reset(const fs::path inPath)
{
    if (isDone())
    {
        this->inFile = std::ifstream(inPath, std::ios::binary);
        if (!inFile)
            throw std::runtime_error("reset()-Error:Failed to open inFile Path:" + inPath.string());
        loadIsDone = false;
    }
    else
        throw std::runtime_error("reset()-Error:inFile is still open, cannot reset to new path:" + inPath.string());
}
void DataLoader::resetByLastReaded()
{
    inFile.seekg(readed, std::ios::beg);
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
    readed += inFile.gcount();
}
void DataLoader::dataLoader(FileSize_uint readSize, std::ifstream &decompressionFile)
{
    try
    {
        std::cout << "[DataLoader DEBUG] dataLoader called with readSize: " << readSize << "\n";
        std::cout << "[DataLoader DEBUG] Current data.size(): " << data.size() << "\n";
        std::cout << "[DataLoader DEBUG] Current data.capacity(): " << data.capacity() << "\n";

        // 清空并重新分配,而不是resize
        data.clear();
        data.resize(readSize);

        std::cout << "[DataLoader DEBUG] After resize, data.size(): " << data.size() << "\n";
        std::cout << "[DataLoader DEBUG] About to read from file...\n";

        decompressionFile.read(reinterpret_cast<char *>(data.data()), readSize);

        std::cout << "[DataLoader DEBUG] Read completed, gcount(): " << decompressionFile.gcount() << "\n";

        data.resize(decompressionFile.gcount());

        std::cout << "[DataLoader DEBUG] Final data.size(): " << data.size() << "\n";
    }
    catch (const std::exception &e)
    {
        std::cout << "[DataLoader DEBUG] Exception caught: " << e.what() << "\n";
        throw std::runtime_error("Error-dataLaoder_decompression()");
    }
}
