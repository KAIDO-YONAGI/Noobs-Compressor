#include "IconHandler.h"
#include <shlobj.h>
#include <shellapi.h>
#include <winuser.h>
#include <windows.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <algorithm>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

// 资源ID定义
#define IDI_APPICON 101

std::wstring IconHandler::Utf8ToWide(const std::string &utf8)
{
    if (utf8.empty())
        return L"";

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    std::vector<wchar_t> buffer(size_needed);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, buffer.data(), size_needed);
    return std::wstring(buffer.data());
}

std::string IconHandler::WideToUtf8(const std::wstring &wide)
{
    if (wide.empty())
        return "";

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, NULL, 0, NULL, NULL);
    std::vector<char> buffer(size_needed);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, buffer.data(), size_needed, NULL, NULL);
    return std::string(buffer.data());
}


bool IconHandler::ExtractIconFromExe(const std::wstring &iconPath)
{
    try
    {
        // 直接使用 ExtractIconEx 从当前 exe 中提取图标
        wchar_t exe_path[MAX_PATH];
        DWORD length = GetModuleFileNameW(NULL, exe_path, MAX_PATH);
        if (length == 0 || length == MAX_PATH)
        {
            return false;
        }

        // 提取第一个大图标（通常是512x512或256x256）
        HICON hIcon = ExtractIconW(GetModuleHandleW(NULL), exe_path, 0);
        if (!hIcon || hIcon == (HICON)1)
        {
            return false;
        }

        // 将HICON保存为ICO文件
        // 获取图标信息
        ICONINFO iconInfo;
        if (!GetIconInfo(hIcon, &iconInfo))
        {
            DestroyIcon(hIcon);
            return false;
        }

        // 获取位图信息
        BITMAP bm = {0};
        if (!GetObjectW(iconInfo.hbmColor, sizeof(BITMAP), &bm))
        {
            DestroyIcon(hIcon);
            return false;
        }

        // 创建ICO文件
        HANDLE hFile = CreateFileW(iconPath.c_str(), GENERIC_WRITE, 0, NULL,
                                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            DestroyIcon(hIcon);
            return false;
        }

        // ICO文件结构
        struct ICONDIR
        {
            WORD idReserved;   // 保留，必须是0
            WORD idType;       // 资源类型，图标为1
            WORD idCount;      // 图像数目
        };

        struct ICONDIRENTRY
        {
            BYTE bWidth;       // 图标宽度
            BYTE bHeight;      // 图标高度
            BYTE bColorCount;  // 调色板颜色数
            BYTE bReserved;    // 保留，必须是0
            WORD wPlanes;      // 位平面数
            WORD wBitCount;    // 每像素的位数
            DWORD dwBytesInRes;// 字节数
            DWORD dwImageOffset;// 文件偏移
        };

        // 简单的ICO文件头（1个图标）
        ICONDIR header;
        header.idReserved = 0;
        header.idType = 1;
        header.idCount = 1;

        DWORD written;
        WriteFile(hFile, &header, sizeof(ICONDIR), &written, NULL);

        // 准备目录项
        ICONDIRENTRY entry = {0};
        entry.bWidth = (bm.bmWidth > 255) ? 0 : (BYTE)bm.bmWidth;
        entry.bHeight = (bm.bmHeight > 255) ? 0 : (BYTE)bm.bmHeight;
        entry.bColorCount = 0;
        entry.bReserved = 0;
        entry.wPlanes = 1;
        entry.wBitCount = 32;
        entry.dwBytesInRes = bm.bmWidth * bm.bmHeight * 4 + 40; // 数据大小（粗估）
        entry.dwImageOffset = sizeof(ICONDIR) + sizeof(ICONDIRENTRY);

        WriteFile(hFile, &entry, sizeof(ICONDIRENTRY), &written, NULL);

        // 写入位图数据（简化版）
        DWORD imageDataSize = bm.bmWidth * bm.bmHeight * 4;
        std::vector<BYTE> imageData(imageDataSize, 0);

        // 从HICON获取像素数据（使用GetDIBits）
        HDC hDC = GetDC(NULL);
        BITMAPINFOHEADER bmiHeader = {0};
        bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmiHeader.biWidth = bm.bmWidth;
        bmiHeader.biHeight = bm.bmHeight;
        bmiHeader.biPlanes = 1;
        bmiHeader.biBitCount = 32;
        bmiHeader.biCompression = BI_RGB;

        GetDIBits(hDC, iconInfo.hbmColor, 0, bm.bmHeight, imageData.data(),
                 (BITMAPINFO*)&bmiHeader, DIB_RGB_COLORS);
        ReleaseDC(NULL, hDC);

        // 写入图标数据
        WriteFile(hFile, imageData.data(), imageDataSize, &written, NULL);

        CloseHandle(hFile);
        DestroyIcon(hIcon);
        DeleteObject(iconInfo.hbmColor);
        DeleteObject(iconInfo.hbmMask);

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool IconHandler::RegisterSyFileIcon(const std::string &iconPath)
{
    try
    {
        std::wstring icon_source;

        // 如果提供了图标文件路径，使用该路径；否则提取到 %APPDATA%
        if (!iconPath.empty())
        {
            icon_source = Utf8ToWide(iconPath);
        }
        else
        {
            // 获取 %APPDATA% 路径
            wchar_t appdata_path[MAX_PATH];
            if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appdata_path) != S_OK)
            {
                return false;
            }

            // 创建应用程序目录
            std::wstring app_dir = std::wstring(appdata_path) + L"\\Secure Files Compressor";
            CreateDirectoryW(app_dir.c_str(), NULL); // 忽略错误，目录可能已存在

            // 图标文件路径
            icon_source = app_dir + L"\\syfile.ico";

            // 检查图标是否已存在，不存在则提取
            if (GetFileAttributesW(icon_source.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                if (!ExtractIconFromExe(icon_source))
                {
                    return false;
                }
            }
        }

        // 第一步: 删除旧的 .sy 关联缓存（强制重新读取）
        HKEY hCacheKey = NULL;
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
                          L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.sy",
                          0,
                          KEY_WRITE,
                          &hCacheKey) == ERROR_SUCCESS)
        {
            RegDeleteValueW(hCacheKey, L"UserChoice");
            RegCloseKey(hCacheKey);
        }

        // 第二步: 设置 .sy 扩展名映射到 syfile 类型（使用当前用户注册表，无需管理员权限）
        HKEY hKey = NULL;
        LONG result = RegCreateKeyExW(HKEY_CURRENT_USER,
                                      L"Software\\Classes\\.sy",
                                      0,
                                      NULL,
                                      REG_OPTION_NON_VOLATILE,
                                      KEY_WRITE,
                                      NULL,
                                      &hKey,
                                      NULL);

        if (result == ERROR_SUCCESS && hKey)
        {
            RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE *)L"syfile",
                          (DWORD)(wcslen(L"syfile") + 1) * sizeof(wchar_t));
            RegCloseKey(hKey);
            hKey = NULL;
        }

        // 第三步: 创建 syfile 类型描述
        result = RegCreateKeyExW(HKEY_CURRENT_USER,
                                 L"Software\\Classes\\syfile",
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_WRITE,
                                 NULL,
                                 &hKey,
                                 NULL);

        if (result == ERROR_SUCCESS && hKey)
        {
            RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE *)L"Secure Files Archive",
                          (DWORD)(wcslen(L"Secure Files Archive") + 1) * sizeof(wchar_t));
            RegCloseKey(hKey);
            hKey = NULL;
        }

        // 第四步: 设置图标资源
        result = RegCreateKeyExW(HKEY_CURRENT_USER,
                                 L"Software\\Classes\\syfile\\DefaultIcon",
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_WRITE,
                                 NULL,
                                 &hKey,
                                 NULL);

        if (result == ERROR_SUCCESS && hKey)
        {
            // 获取exe的完整路径
            wchar_t exe_path[MAX_PATH];
            DWORD length = GetModuleFileNameW(NULL, exe_path, MAX_PATH);

            if (length > 0 && length < MAX_PATH)
            {
                // 使用exe路径作为图标源，指向第一个图标（索引0）
                std::wstring icon_resource = std::wstring(exe_path) + L",0";
                RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE *)icon_resource.c_str(),
                              (DWORD)(icon_resource.length() + 1) * sizeof(wchar_t));
            }
            else
            {
                // 备用方案：使用提取的ico文件
                RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE *)icon_source.c_str(),
                              (DWORD)(icon_source.length() + 1) * sizeof(wchar_t));
            }

            RegCloseKey(hKey);
            hKey = NULL;
        }
        else
        {
            return false;
        }

        // 第五步: 通知 Windows 更新文件关联缓存
        // 使用多种方式确保缓存被刷新
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_FLUSH, NULL, NULL);
        Sleep(50);
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool IconHandler::AssociateIconToSyFile(const std::string &syFilePath, const std::string &iconPath)
{
    try
    {
        // 在注册表中注册图标关联（直接使用exe文件作为图标源）
        if (!RegisterSyFileIcon(iconPath))
        {
            return false;
        }

        // 通知Windows更新文件关联缓存（多种方式）
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_FLUSH, NULL, NULL);

        return true;
    }
    catch (...)
    {
        return false;
    }
}
