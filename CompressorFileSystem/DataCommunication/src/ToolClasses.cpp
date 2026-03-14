#include "../include/ToolClasses.h"

namespace fs = std::filesystem;

// 将路径转换为 Windows 长路径格式（添加 \\?\ 前缀）
// 这样可以突破 260 字符的限制
static std::wstring toLongPath(const std::filesystem::path& p)
{
    std::wstring pathStr = p.wstring();

    // 如果路径已经以 \\?\ 开头，直接返回
    if (pathStr.rfind(L"\\\\?\\", 0) == 0)
        return pathStr;

    // 如果路径超过 240 字符，添加 \\?\ 前缀
    if (pathStr.size() >= 240)
    {
        // 对于绝对路径 (C:\...)
        if (pathStr.size() >= 2 && pathStr[1] == L':')
        {
            return L"\\\\?\\" + pathStr;
        }
        // 对于网络路径 (\\server\share)
        else if (pathStr.size() >= 2 && pathStr[0] == L'\\' && pathStr[1] == L'\\')
        {
            return L"\\\\?\\UNC\\" + pathStr.substr(2);
        }
    }

    return pathStr;
}

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