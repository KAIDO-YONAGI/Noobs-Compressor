#pragma once

#include "FileLibrary.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace Y_flib
{
    /**
     * 文件系统条目的统一元数据。
     *
     * Windows 下由 Win32 API 填充，避免 MinGW 的 std::filesystem
     * 在路径超过 MAX_PATH 后把正常文件误判为不存在。
     */
    struct FileSystemEntryInfo
    {
        bool exists = false;
        bool isRegularFile = false;
        bool isDirectory = false;
        bool isSymbolicLink = false;
        Y_flib::FileSize size = 0;
    };

    /**
     * 跨平台文件系统兼容工具。
     *
     * 对外始终保留普通业务路径；只有真正访问磁盘时，Windows 才转换为
     * \\?\ 扩展长度路径，并使用不受 MAX_PATH 限制的 Win32 API。
     */
    class FileSystemUtils
    {
    public:
        // Windows 返回绝对扩展长度路径，其他平台保持原路径不变。
        static std::filesystem::path pathForIo(const std::filesystem::path &path);

        // 查询条目类型和文件大小，不依赖 MinGW 中受 MAX_PATH 限制的 status。
        static FileSystemEntryInfo queryEntry(const std::filesystem::path &path);

        // 枚举目录直属子项，返回未带 \\?\ 前缀的业务路径。
        static std::vector<std::filesystem::path> listDirectory(
            const std::filesystem::path &directory);

        static bool exists(const std::filesystem::path &path);

        // 创建单层目录；目录已存在时返回 false。
        static bool createDirectory(const std::filesystem::path &path);

        // 递归创建目录；目标目录原本已存在时返回 false。
        static bool createDirectories(const std::filesystem::path &path);

        // 递归删除条目并返回删除数量；目录符号链接不会跟随到链接目标。
        static std::uintmax_t removeAll(const std::filesystem::path &path);
    };
} // namespace Y_flib
