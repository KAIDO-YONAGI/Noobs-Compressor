// Transfer.h
#ifndef TRANSFER_H
#define TRANSFER_H

#include "../include/FileLibrary.h"

// 使用inline避免重复定义
inline std::wstring convertToWString(const std::string &s)
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

// 使用inline
inline fs::path _getPath(const std::string &p)
{
    std::wstring wPath = convertToWString(p);
    return fs::path(wPath);
}

#endif