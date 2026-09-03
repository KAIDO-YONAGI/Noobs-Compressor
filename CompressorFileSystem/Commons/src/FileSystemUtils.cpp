#include "../include/FileSystemUtils.h"

#include <system_error>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Y_flib
{
    std::filesystem::path FileSystemUtils::pathForIo(const std::filesystem::path &path)
    {
#ifndef _WIN32
        return path;
#else
        if (path.empty())
        {
            return {};
        }

        const std::wstring original = path.native();
        if (original.starts_with(LR"(\\?\)") || original.starts_with(LR"(\\.\)"))
        {
            // MinGW 可能无法正确判断扩展长度路径是否为绝对路径，因此先行返回。
            return path;
        }

        std::filesystem::path absolutePath =
            path.is_absolute() ? path : std::filesystem::absolute(path);
        absolutePath = absolutePath.lexically_normal();
        absolutePath.make_preferred();

        const std::wstring native = absolutePath.native();
        if (native.starts_with(LR"(\\)"))
        {
            // UNC 路径的扩展长度格式为 \\?\UNC\server\share\...
            return std::filesystem::path(
                std::wstring(LR"(\\?\UNC\)") + native.substr(2));
        }

        return std::filesystem::path(std::wstring(LR"(\\?\)") + native);
#endif
    }

    FileSystemEntryInfo FileSystemUtils::queryEntry(const std::filesystem::path &path)
    {
        FileSystemEntryInfo info;

#ifdef _WIN32
        const std::filesystem::path ioPath = pathForIo(path);
        WIN32_FILE_ATTRIBUTE_DATA attributes{};
        // GetFileAttributesExW 可同时获得类型与 64 位文件大小，且支持扩展长度路径。
        if (!GetFileAttributesExW(ioPath.c_str(), GetFileExInfoStandard, &attributes))
        {
            const DWORD error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND ||
                error == ERROR_PATH_NOT_FOUND ||
                error == ERROR_INVALID_NAME)
            {
                return info;
            }

            throw std::filesystem::filesystem_error(
                "Failed to query filesystem entry",
                path,
                std::error_code(static_cast<int>(error), std::system_category()));
        }

        info.exists = true;
        info.isDirectory =
            (attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        info.isSymbolicLink =
            (attributes.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
        info.isRegularFile = !info.isDirectory;
        info.size =
            (static_cast<Y_flib::FileSize>(attributes.nFileSizeHigh) << 32) |
            static_cast<Y_flib::FileSize>(attributes.nFileSizeLow);
#else
        std::error_code error;
        const std::filesystem::file_status linkStatus =
            std::filesystem::symlink_status(path, error);
        if (error)
        {
            if (error == std::errc::no_such_file_or_directory)
            {
                return info;
            }
            throw std::filesystem::filesystem_error(
                "Failed to query filesystem entry", path, error);
        }

        info.exists = std::filesystem::exists(linkStatus);
        if (!info.exists)
        {
            return info;
        }

        info.isSymbolicLink = std::filesystem::is_symlink(linkStatus);
        const std::filesystem::file_status targetStatus =
            std::filesystem::status(path, error);
        if (!error)
        {
            info.isRegularFile = std::filesystem::is_regular_file(targetStatus);
            info.isDirectory = std::filesystem::is_directory(targetStatus);
            if (info.isRegularFile)
            {
                info.size = std::filesystem::file_size(path);
            }
        }
        else if (!info.isSymbolicLink)
        {
            throw std::filesystem::filesystem_error(
                "Failed to query filesystem entry", path, error);
        }
#endif

        return info;
    }

    bool FileSystemUtils::exists(const std::filesystem::path &path)
    {
        return queryEntry(path).exists;
    }

    std::vector<std::filesystem::path> FileSystemUtils::listDirectory(
        const std::filesystem::path &directory)
    {
        std::vector<std::filesystem::path> entries;

#ifdef _WIN32
        const std::filesystem::path searchPath = pathForIo(directory) / L"*";
        WIN32_FIND_DATAW findData{};
        // directory_iterator 在 MinGW 下仍受 MAX_PATH 影响，改用原生宽字符枚举。
        HANDLE findHandle = FindFirstFileW(searchPath.c_str(), &findData);
        if (findHandle == INVALID_HANDLE_VALUE)
        {
            const DWORD error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND)
            {
                return entries;
            }

            throw std::filesystem::filesystem_error(
                "Failed to enumerate directory",
                directory,
                std::error_code(static_cast<int>(error), std::system_category()));
        }

        try
        {
            do
            {
                const std::wstring_view name(findData.cFileName);
                if (name != L"." && name != L"..")
                {
                    // 保留调用方传入的逻辑路径，避免 \\?\ 前缀写入归档元数据。
                    entries.push_back(directory / std::filesystem::path(name));
                }
            } while (FindNextFileW(findHandle, &findData));

            const DWORD error = GetLastError();
            if (error != ERROR_NO_MORE_FILES)
            {
                throw std::filesystem::filesystem_error(
                    "Failed while enumerating directory",
                    directory,
                    std::error_code(static_cast<int>(error), std::system_category()));
            }
        }
        catch (...)
        {
            FindClose(findHandle);
            throw;
        }

        FindClose(findHandle);
#else
        for (const std::filesystem::directory_entry &entry :
             std::filesystem::directory_iterator(directory))
        {
            entries.push_back(entry.path());
        }
#endif

        return entries;
    }

    bool FileSystemUtils::createDirectory(const std::filesystem::path &path)
    {
#ifdef _WIN32
        const std::filesystem::path ioPath = pathForIo(path);
        if (CreateDirectoryW(ioPath.c_str(), nullptr))
        {
            return true;
        }

        const DWORD error = GetLastError();
        if (error == ERROR_ALREADY_EXISTS && queryEntry(path).isDirectory)
        {
            return false;
        }

        throw std::filesystem::filesystem_error(
            "Failed to create directory",
            path,
            std::error_code(static_cast<int>(error), std::system_category()));
#else
        return std::filesystem::create_directory(path);
#endif
    }

    bool FileSystemUtils::createDirectories(const std::filesystem::path &path)
    {
        if (path.empty())
        {
            return false;
        }

        const FileSystemEntryInfo current = queryEntry(path);
        if (current.exists)
        {
            if (!current.isDirectory)
            {
                throw std::filesystem::filesystem_error(
                    "Cannot create directory because a non-directory entry exists",
                    path,
                    std::make_error_code(std::errc::file_exists));
            }
            return false;
        }

        const std::filesystem::path parent = path.parent_path();
        if (!parent.empty() && parent != path)
        {
            const FileSystemEntryInfo parentInfo = queryEntry(parent);
            if (!parentInfo.exists)
            {
                createDirectories(parent);
            }
            else if (!parentInfo.isDirectory)
            {
                throw std::filesystem::filesystem_error(
                    "Cannot create directory because its parent is not a directory",
                    parent,
                    std::make_error_code(std::errc::not_a_directory));
            }
        }

        return createDirectory(path);
    }

    std::uintmax_t FileSystemUtils::removeAll(const std::filesystem::path &path)
    {
        const FileSystemEntryInfo info = queryEntry(path);
        if (!info.exists)
        {
            return 0;
        }

#ifdef _WIN32
        std::uintmax_t removed = 0;
        if (info.isDirectory)
        {
            // 重解析点可能指向目录树外部；删除链接本身，但不递归链接目标。
            if (!info.isSymbolicLink)
            {
                for (const std::filesystem::path &child : listDirectory(path))
                {
                    removed += removeAll(child);
                }
            }

            if (!RemoveDirectoryW(pathForIo(path).c_str()))
            {
                const DWORD error = GetLastError();
                throw std::filesystem::filesystem_error(
                    "Failed to remove directory",
                    path,
                    std::error_code(static_cast<int>(error), std::system_category()));
            }
        }
        else
        {
            const std::filesystem::path ioPath = pathForIo(path);
            const DWORD attributes = GetFileAttributesW(ioPath.c_str());
            if (attributes != INVALID_FILE_ATTRIBUTES &&
                (attributes & FILE_ATTRIBUTE_READONLY) != 0)
            {
                SetFileAttributesW(ioPath.c_str(), attributes & ~FILE_ATTRIBUTE_READONLY);
            }

            if (!DeleteFileW(ioPath.c_str()))
            {
                const DWORD error = GetLastError();
                throw std::filesystem::filesystem_error(
                    "Failed to remove file",
                    path,
                    std::error_code(static_cast<int>(error), std::system_category()));
            }
        }

        return removed + 1;
#else
        return std::filesystem::remove_all(path);
#endif
    }
} // namespace Y_flib
