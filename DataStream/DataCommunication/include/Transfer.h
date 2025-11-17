// Transfer.h
#ifndef TRANSFER
#define TRANSFER

#include "../include/FileLibrary.h"

// 将字节字符串转换为 Wide 字符串
std::wstring convertToWString(const std::string &s)
{
    std::setlocale(LC_ALL, ""); // 使用本地化设置
    size_t len = s.size() + 1;
    wchar_t *wStr = new wchar_t[len];
    size_t result = std::mbstowcs(wStr, s.c_str(), len);
    if (result == (size_t)-1)
    {
        delete[] wStr;
        throw std::runtime_error("mbstowcs conversion failed");
    }
    std::wstring ws(wStr);
    delete[] wStr;
    return ws;
}

// 处理路径的函数
fs::path _getPath(const std::string &p)
{
    std::wstring wPath = convertToWString(p);
    return fs::path(wPath);
}

#endif