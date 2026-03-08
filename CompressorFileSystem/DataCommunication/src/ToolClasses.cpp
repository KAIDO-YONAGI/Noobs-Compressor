#include "../include/ToolClasses.h"

namespace fs = std::filesystem;

std::filesystem::path PathTransfer::transPath(const std::string& p)
{
    return fs::path(p);
}


void Locator::locateFromBegin(std::ofstream& outFile, Y_flib::FileSize offset)
{
    if (!outFile)
        throw std::runtime_error("locateFromBegin(): invalid ofstream");

    outFile.clear();
    outFile.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
}

void Locator::locateFromBegin(std::ifstream& inFile, Y_flib::FileSize offset)
{
    if (!inFile)
        throw std::runtime_error("locateFromBegin(): invalid ifstream");

    inFile.clear();
    inFile.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
}

void Locator::locateFromBegin(std::fstream& file, Y_flib::FileSize offset)
{
    if (!file)
        throw std::runtime_error("locateFromBegin(): invalid fstream");

    file.clear();
    file.seekg(offset, std::ios::beg);
    file.seekp(offset, std::ios::beg);
}

void Locator::locateFromEnd(std::ofstream& outFile, Y_flib::FileSize offset)
{
    if (!outFile)
        throw std::runtime_error("locateFromEnd(): invalid ofstream");

    outFile.clear();
    outFile.seekp(static_cast<std::streamoff>(offset), std::ios::end);
}

void Locator::locateFromEnd(std::ifstream& inFile, Y_flib::FileSize offset)
{
    if (!inFile)
        throw std::runtime_error("locateFromEnd(): invalid ifstream");

    inFile.clear();
    inFile.seekg(static_cast<std::streamoff>(offset), std::ios::end);
}

void Locator::locateFromEnd(std::fstream& file, Y_flib::FileSize offset)
{
    if (!file)
        throw std::runtime_error("locateFromEnd(): invalid fstream");

    file.clear();
    file.seekg(offset, std::ios::end);
    file.seekp(offset, std::ios::end);
}

Y_flib::FileSize Locator::getFileSize(
    const fs::path& filePathToScan,
    std::ofstream& outFile)
{
    try
    {
        outFile.flush();
        return fs::file_size(filePathToScan);
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "getFileSize() Error: " << e.what() << '\n';
        return 0;
    }
}