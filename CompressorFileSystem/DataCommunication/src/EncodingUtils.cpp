#include "../include/EncodingUtils.h"

#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <codecvt>
#include <locale>
#endif

namespace Y_flib
{
std::filesystem::path EncodingUtils::pathFromUtf8(std::string_view utf8)
{
    if (utf8.empty())
    {
        return {};
    }

#ifdef _WIN32
    return std::filesystem::path(utf8ToWide(utf8));
#else
    return std::filesystem::path(std::string(utf8));
#endif
}

std::string EncodingUtils::pathToUtf8(const std::filesystem::path &path)
{
#ifdef _WIN32
    return wideToUtf8(path.native());
#else
    return u8ToString(path.u8string());
#endif
}

std::wstring EncodingUtils::utf8ToWide(std::string_view utf8)
{
    if (utf8.empty())
    {
        return {};
    }

#ifdef _WIN32
    const int inputSize = static_cast<int>(utf8.size());
    int wideSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), inputSize, nullptr, 0);
    if (wideSize <= 0)
    {
        wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), inputSize, nullptr, 0);
    }
    if (wideSize <= 0)
    {
        throw std::runtime_error("Failed to convert UTF-8 text to wide text");
    }

    std::wstring wide(static_cast<size_t>(wideSize), L'\0');
    const int converted = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), inputSize, wide.data(), wideSize);
    if (converted <= 0)
    {
        throw std::runtime_error("Failed to convert UTF-8 text to wide text");
    }

    return wide;
#else
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(utf8.data(), utf8.data() + utf8.size());
#endif
}

std::string EncodingUtils::wideToUtf8(std::wstring_view wide)
{
    if (wide.empty())
    {
        return {};
    }

#ifdef _WIN32
    const int inputSize = static_cast<int>(wide.size());
    const int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), inputSize, nullptr, 0, nullptr, nullptr);
    if (utf8Size <= 0)
    {
        throw std::runtime_error("Failed to convert wide text to UTF-8 text");
    }

    std::string utf8(static_cast<size_t>(utf8Size), '\0');
    const int converted = WideCharToMultiByte(CP_UTF8, 0, wide.data(), inputSize, utf8.data(), utf8Size, nullptr, nullptr);
    if (converted <= 0)
    {
        throw std::runtime_error("Failed to convert wide text to UTF-8 text");
    }

    return utf8;
#else
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(wide.data(), wide.data() + wide.size());
#endif
}

std::string EncodingUtils::u8ToString(std::u8string_view u8str)
{
    std::string result;
    result.reserve(u8str.size());

    for (char8_t ch : u8str)
    {
        result.push_back(static_cast<char>(ch));
    }

    return result;
}

#if SFC_HAS_QSTRING
std::string EncodingUtils::qStringToUtf8(const QString &value)
{
    return value.toUtf8().toStdString();
}

QString EncodingUtils::utf8ToQString(std::string_view utf8)
{
    return QString::fromUtf8(utf8.data(), static_cast<int>(utf8.size()));
}

std::filesystem::path EncodingUtils::qStringToPath(const QString &value)
{
    return pathFromUtf8(qStringToUtf8(value));
}

QString EncodingUtils::pathToQString(const std::filesystem::path &path)
{
    return utf8ToQString(pathToUtf8(path));
}
#endif
} // namespace Y_flib
