#pragma once

#include "FileLibrary.h"

#include <cstdint>
#include <filesystem>
#include <vector>

#ifndef _WIN32
#error "Simple Files Compressor now supports Windows filesystem semantics only."
#endif

namespace Y_flib
{
    enum class WindowsLinkType : std::uint8_t
    {
        None = 0,
        SymbolicLink,
        Junction,
        Unsupported
    };

    /**
     * 文件系统条目的统一元数据。
     *
     * 由 Win32 API 填充，重解析点不会再同时被当作普通目录或文件，
     * 因而目录扫描遇到符号链接/Junction 时不会进入其目标。
     */
    struct FileSystemEntryInfo
    {
        bool exists = false;
        bool isRegularFile = false;
        bool isDirectory = false;
        bool isReparsePoint = false;
        Y_flib::FileSize size = 0;
    };

    struct WindowsLinkInfo
    {
        WindowsLinkType type = WindowsLinkType::None;
        bool targetIsDirectory = false;
        // 保存重解析点自身记录的目标字符串，不解析、不验证目标是否存在。
        std::filesystem::path targetPath;
    };

    /**
     * Windows 文件系统兼容工具。
     *
     * 对外始终保留普通业务路径；真正访问磁盘时转换为 \\?\ 扩展长度路径，
     * 并通过重解析点 API 读取和重建符号链接/Junction。
     */
    class FileSystemUtils
    {
    public:
        // 返回绝对扩展长度路径。
        static std::filesystem::path pathForIo(const std::filesystem::path &path);

        // 查询条目类型和文件大小，不依赖 MinGW 中受 MAX_PATH 限制的 status。
        static FileSystemEntryInfo queryEntry(const std::filesystem::path &path);

        // 读取链接自身的类型和目标字符串，不访问链接目标。
        static WindowsLinkInfo readLinkForArchive(
            const std::filesystem::path &linkPath);

        // 按归档中的原始目标字符串创建 Windows 符号链接或 Junction。
        static void createLink(
            const std::filesystem::path &linkPath,
            const std::filesystem::path &targetPath,
            Y_flib::FlagType linkType);

        // 枚举目录直属子项，返回未带 \\?\ 前缀的业务路径。
        static std::vector<std::filesystem::path> listDirectory(
            const std::filesystem::path &directory);

        static bool exists(const std::filesystem::path &path);

        // 创建单层目录；目录已存在时返回 false。
        static bool createDirectory(const std::filesystem::path &path);

        // 递归创建目录；目标目录原本已存在时返回 false。
        static bool createDirectories(const std::filesystem::path &path);

        // 递归删除条目并返回删除数量；任何重解析点都只删除链接本身。
        static std::uintmax_t removeAll(const std::filesystem::path &path);
    };
} // namespace Y_flib
