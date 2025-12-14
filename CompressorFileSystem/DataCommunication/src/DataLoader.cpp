#include "../include/DataLoader.h"

void DataLoader::done()
{
    if (inFile.is_open())
    {
        inFile.close();
    }
    buffer.clear();
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
void DataLoader::dataLoader(FileSize_uint readSize)
{
    try
    {
        buffer.resize(readSize);
        inFile.read(buffer.data(), readSize);
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