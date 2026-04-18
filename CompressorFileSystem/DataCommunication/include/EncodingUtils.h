#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#if __has_include(<QString>)
#include <QString>
#define SFC_HAS_QSTRING 1
#else
#define SFC_HAS_QSTRING 0
#endif

/*
 * EncodingUtils - 项目统一的编码/路径转换入口
 *
 * 调用须知:
 * 1. 核心层对外传递 std::string 路径时，统一约定为 UTF-8。
 * 2. 需要进入 std::filesystem 时，统一调用 pathFromUtf8()；
 *    需要从 std::filesystem 回到文本时，统一调用 pathToUtf8()。
 * 3. Qt 边界不要再直接混用 QString::fromStdString() / toStdString() 处理路径，
 *    因为这里的 std::string 在本项目里按 UTF-8 解释，而不是本地代码页。
 * 4. utf8ToWide() / wideToUtf8() 只建议用于 Windows API 边界；
 *    普通业务代码优先使用 QString 或 std::filesystem::path。
 * 5. u8ToString() 只用于承接 char8_t / std::u8string 数据，
 *    典型场景是 std::filesystem::path::u8string()。
 */
namespace Y_flib
{
class EncodingUtils
{
public:
    // 调用须知: 输入必须是 UTF-8 编码文本；返回值可直接用于文件系统访问。
    static std::filesystem::path pathFromUtf8(std::string_view utf8);

    // 调用须知: 返回 UTF-8 编码字符串，适合继续传给核心层、日志或 Qt 转换层。
    static std::string pathToUtf8(const std::filesystem::path &path);

    // 调用须知: 仅在需要调用 Windows 宽字符 API 时使用。
    static std::wstring utf8ToWide(std::string_view utf8);

    // 调用须知: 仅在宽字符 API 返回后需要回到 UTF-8 文本时使用。
    static std::string wideToUtf8(std::wstring_view wide);

    // 调用须知: 仅做 char8_t -> char 的字节搬运，前提是源数据本身就是 UTF-8。
    static std::string u8ToString(std::u8string_view u8str);

#if SFC_HAS_QSTRING
    // 调用须知: Qt 层把 QString 交给核心层前，统一走这里得到 UTF-8。
    static std::string qStringToUtf8(const QString &value);

    // 调用须知: 核心层返回 UTF-8 文本给 Qt 展示时，统一走这里。
    static QString utf8ToQString(std::string_view utf8);

    // 调用须知: Qt 路径进入 std::filesystem 前统一走这里，避免重复手写转换。
    static std::filesystem::path qStringToPath(const QString &value);

    // 调用须知: std::filesystem 路径回到 Qt 控件显示时统一走这里。
    static QString pathToQString(const std::filesystem::path &path);
#endif
};
} // namespace Y_flib
