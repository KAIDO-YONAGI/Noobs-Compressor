#include "../include/ToolClasses.h"

#ifdef _WIN32
#include <windows.h>
#endif

std::filesystem::path PathTransfer::transPath(std::string_view p)
{
#ifdef _WIN32
    // Windows: 将 UTF-8 字符串转换为宽字符，再创建 path
    // 这样可以正确处理中文路径
    if (p.empty()) {
        return std::filesystem::path();
    }

    // MultiByteToWideChar 需要 int 类型的大小
    int inputSize = static_cast<int>(p.size());

    // 计算需要的宽字符缓冲区大小
    int wideSize = MultiByteToWideChar(
        CP_UTF8,           // 假设输入是 UTF-8
        0,
        p.data(),
        inputSize,
        nullptr,
        0
    );

    if (wideSize <= 0) {
        // UTF-8 转换失败，尝试系统默认编码 (GBK)
        wideSize = MultiByteToWideChar(
            CP_ACP,            // 系统默认编码
            0,
            p.data(),
            inputSize,
            nullptr,
            0
        );
    }

    if (wideSize > 0) {
        std::wstring widePath(wideSize, 0);
        MultiByteToWideChar(
            CP_UTF8,
            0,
            p.data(),
            inputSize,
            widePath.data(),
            wideSize
        );
        return std::filesystem::path(widePath);
    }
#endif
    return std::filesystem::path(p);
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
    const std::filesystem::path& filePathToScan,
    std::ofstream& outFile)
{
    try
    {
        outFile.flush();
        return std::filesystem::file_size(filePathToScan);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "getFileSize() Error: " << e.what() << '\n';
        return 0;
    }
}