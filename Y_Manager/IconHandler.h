#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <thread>

/**
 * 图标处理类 - 处理从EXE中提取和应用图标功能
 */
class IconHandler
{
public:
    /**
     * 为.sy文件关联图标（Windows快捷方式方法）
     * 注意：直接使用exe文件中的图标资源，无需单独的图标文件
     * @param syFilePath .sy文件的完整路径（可为空）
     * @param iconPath 图标文件的完整路径（可为空）
     * @return 成功返回true，失败返回false
     */
    static bool AssociateIconToSyFile(const std::string &syFilePath, const std::string &iconPath);

private:
    /**
     * 将UTF-8字符串转换为宽字符串
     */
    static std::wstring Utf8ToWide(const std::string &utf8);

    /**
     * 将宽字符串转换为UTF-8字符串
     */
    static std::string WideToUtf8(const std::wstring &wide);

    /**
     * 注册.sy文件的图标（在Windows注册表中）
     */
    static bool RegisterSyFileIcon(const std::string &iconPath);

    /**
     * 从exe资源中提取图标到指定文件
     * @param iconPath 输出的.ico文件路径
     * @return 成功返回true，失败返回false
     */
    static bool ExtractIconFromExe(const std::wstring &iconPath);
};
