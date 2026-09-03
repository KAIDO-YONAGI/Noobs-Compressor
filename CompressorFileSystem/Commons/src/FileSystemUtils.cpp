#include "../include/FileSystemUtils.h"
#include "../include/EncodingUtils.h"

#include <windows.h>
#include <winioctl.h>

#include <cstring>
#include <limits>
#include <system_error>
#include <vector>

namespace
{
    using Y_flib::FlagType;
    using Y_flib::WindowsLinkInfo;
    namespace fs = std::filesystem;

    class UniqueHandle
    {
    private:
        HANDLE handle = INVALID_HANDLE_VALUE;

    public:
        explicit UniqueHandle(HANDLE value) : handle(value) {}
        ~UniqueHandle()
        {
            if (handle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(handle);
            }
        }

        UniqueHandle(const UniqueHandle &) = delete;
        UniqueHandle &operator=(const UniqueHandle &) = delete;

        HANDLE get() const { return handle; }
    };

    [[noreturn]] void throwFilesystemError(
        const char *operation,
        const fs::path &path,
        DWORD error = GetLastError())
    {
        throw fs::filesystem_error(
            operation,
            path,
            std::error_code(static_cast<int>(error), std::system_category()));
    }

    fs::path stripExtendedPrefix(const fs::path &path)
    {
        const std::wstring native = path.native();
        if (native.starts_with(LR"(\\?\UNC\)"))
        {
            return fs::path(std::wstring(LR"(\\)") + native.substr(8));
        }
        if (native.starts_with(LR"(\\?\)"))
        {
            return fs::path(native.substr(4));
        }
        return path;
    }

    fs::path normalizeAbsolutePath(const fs::path &path)
    {
        if (path.empty())
        {
            return {};
        }

        fs::path businessPath = stripExtendedPrefix(path);
        if (!businessPath.is_absolute())
        {
            businessPath = fs::absolute(businessPath);
        }
        businessPath = businessPath.lexically_normal();
        businessPath.make_preferred();
        return businessPath;
    }

    HANDLE openPathHandle(const fs::path &path, bool openReparsePoint, DWORD access = 0)
    {
        DWORD flags = FILE_FLAG_BACKUP_SEMANTICS;
        if (openReparsePoint)
        {
            flags |= FILE_FLAG_OPEN_REPARSE_POINT;
        }

        return CreateFileW(
            Y_flib::FileSystemUtils::pathForIo(path).c_str(),
            access,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            flags,
            nullptr);
    }

    template <typename T>
    T readValue(const std::vector<unsigned char> &buffer, size_t offset)
    {
        if (offset + sizeof(T) > buffer.size())
        {
            throw std::runtime_error("Invalid Windows reparse data buffer");
        }

        T value{};
        std::memcpy(&value, buffer.data() + offset, sizeof(T));
        return value;
    }

    template <typename T>
    void writeValue(std::vector<unsigned char> &buffer, size_t offset, T value)
    {
        if (offset + sizeof(T) > buffer.size())
        {
            throw std::runtime_error("Windows reparse data buffer overflow");
        }
        std::memcpy(buffer.data() + offset, &value, sizeof(T));
    }

    std::wstring readReparsePath(
        const std::vector<unsigned char> &buffer,
        size_t pathBufferOffset,
        USHORT byteOffset,
        USHORT byteLength)
    {
        const size_t begin = pathBufferOffset + byteOffset;
        const size_t end = begin + byteLength;
        if ((byteLength % sizeof(wchar_t)) != 0 || end > buffer.size())
        {
            throw std::runtime_error("Invalid path range in Windows reparse data");
        }

        return std::wstring(
            reinterpret_cast<const wchar_t *>(buffer.data() + begin),
            byteLength / sizeof(wchar_t));
    }

    std::wstring normalizeNtTarget(std::wstring target)
    {
        if (target.starts_with(LR"(\??\UNC\)"))
        {
            return std::wstring(LR"(\\)") + target.substr(8);
        }
        if (target.starts_with(LR"(\??\)"))
        {
            return target.substr(4);
        }
        if (target.starts_with(LR"(\\?\UNC\)"))
        {
            return std::wstring(LR"(\\)") + target.substr(8);
        }
        if (target.starts_with(LR"(\\?\)"))
        {
            return target.substr(4);
        }
        return target;
    }

    WindowsLinkInfo readRawLink(
        const fs::path &linkPath,
        bool targetIsDirectory)
    {
        UniqueHandle handle(openPathHandle(linkPath, true));
        if (handle.get() == INVALID_HANDLE_VALUE)
        {
            throwFilesystemError("Failed to open Windows reparse point", linkPath);
        }

        std::vector<unsigned char> buffer(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
        DWORD bytesReturned = 0;
        if (!DeviceIoControl(
                handle.get(),
                FSCTL_GET_REPARSE_POINT,
                nullptr,
                0,
                buffer.data(),
                static_cast<DWORD>(buffer.size()),
                &bytesReturned,
                nullptr))
        {
            throwFilesystemError("Failed to read Windows reparse point", linkPath);
        }
        buffer.resize(bytesReturned);

        const ULONG tag = readValue<ULONG>(buffer, 0);
        size_t pathBufferOffset = 0;
        USHORT substituteOffset = 0;
        USHORT substituteLength = 0;
        USHORT printOffset = 0;
        USHORT printLength = 0;
        FlagType linkType;

        if (tag == IO_REPARSE_TAG_SYMLINK)
        {
            // 符号链接的标签相同，必须结合目录属性确定解压时使用的创建标志。
            linkType = targetIsDirectory
                           ? FlagType::SymbolicLinkDirectory
                           : FlagType::SymbolicLinkFile;
            substituteOffset = readValue<USHORT>(buffer, 8);
            substituteLength = readValue<USHORT>(buffer, 10);
            printOffset = readValue<USHORT>(buffer, 12);
            printLength = readValue<USHORT>(buffer, 14);
            pathBufferOffset = 20;
        }
        else if (tag == IO_REPARSE_TAG_MOUNT_POINT)
        {
            linkType = FlagType::Junction;
            substituteOffset = readValue<USHORT>(buffer, 8);
            substituteLength = readValue<USHORT>(buffer, 10);
            printOffset = readValue<USHORT>(buffer, 12);
            printLength = readValue<USHORT>(buffer, 14);
            pathBufferOffset = 16;
        }
        else
        {
            throw std::runtime_error(
                "Unsupported Windows reparse point tag " +
                std::to_string(static_cast<unsigned long>(tag)));
        }

        std::wstring rawTarget;
        if (printLength != 0)
        {
            rawTarget = readReparsePath(
                buffer, pathBufferOffset, printOffset, printLength);
        }
        else
        {
            rawTarget = readReparsePath(
                buffer, pathBufferOffset, substituteOffset, substituteLength);
        }

        if (rawTarget.empty())
        {
            throw std::runtime_error("Windows link target is empty");
        }
        return WindowsLinkInfo{
            linkType,
            fs::path(normalizeNtTarget(std::move(rawTarget)))};
    }

    void createSymbolicLink(
        const fs::path &linkPath,
        const fs::path &targetPath,
        bool targetIsDirectory)
    {
        DWORD flags = targetIsDirectory ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;
        flags |= SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;

        if (CreateSymbolicLinkW(
                Y_flib::FileSystemUtils::pathForIo(linkPath).c_str(),
                targetPath.c_str(),
                flags))
        {
            return;
        }

        DWORD error = GetLastError();
        if (error == ERROR_INVALID_PARAMETER)
        {
            // 较旧 Windows 不认识无特权创建标志，回退到传统权限模型。
            flags &= ~SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
            if (CreateSymbolicLinkW(
                    Y_flib::FileSystemUtils::pathForIo(linkPath).c_str(),
                    targetPath.c_str(),
                    flags))
            {
                return;
            }
            error = GetLastError();
        }

        throwFilesystemError("Failed to create Windows symbolic link", linkPath, error);
    }

    void createJunction(const fs::path &linkPath, const fs::path &absoluteTarget)
    {
        const fs::path normalizedTarget = normalizeAbsolutePath(absoluteTarget);
        const std::wstring printName = normalizedTarget.native();
        if (printName.starts_with(LR"(\\)"))
        {
            throw std::runtime_error("Windows Junction does not support UNC targets");
        }

        const std::wstring substituteName = std::wstring(LR"(\??\)") + printName;
        const size_t substituteBytes = substituteName.size() * sizeof(wchar_t);
        const size_t printBytes = printName.size() * sizeof(wchar_t);
        const size_t pathBytes =
            substituteBytes + sizeof(wchar_t) + printBytes + sizeof(wchar_t);
        const size_t dataLength = 8 + pathBytes;
        const size_t totalLength = 8 + dataLength;

        if (dataLength > std::numeric_limits<USHORT>::max() ||
            totalLength > MAXIMUM_REPARSE_DATA_BUFFER_SIZE)
        {
            throw std::runtime_error("Windows Junction target path is too long");
        }

        Y_flib::FileSystemUtils::createDirectory(linkPath);
        try
        {
            UniqueHandle handle(openPathHandle(linkPath, true, GENERIC_WRITE));
            if (handle.get() == INVALID_HANDLE_VALUE)
            {
                throwFilesystemError("Failed to open Junction directory", linkPath);
            }

            std::vector<unsigned char> buffer(totalLength, 0);
            writeValue<ULONG>(buffer, 0, IO_REPARSE_TAG_MOUNT_POINT);
            writeValue<USHORT>(buffer, 4, static_cast<USHORT>(dataLength));
            writeValue<USHORT>(buffer, 8, 0);
            writeValue<USHORT>(buffer, 10, static_cast<USHORT>(substituteBytes));
            writeValue<USHORT>(
                buffer,
                12,
                static_cast<USHORT>(substituteBytes + sizeof(wchar_t)));
            writeValue<USHORT>(buffer, 14, static_cast<USHORT>(printBytes));

            std::memcpy(buffer.data() + 16, substituteName.data(), substituteBytes);
            std::memcpy(
                buffer.data() + 16 + substituteBytes + sizeof(wchar_t),
                printName.data(),
                printBytes);

            DWORD bytesReturned = 0;
            if (!DeviceIoControl(
                    handle.get(),
                    FSCTL_SET_REPARSE_POINT,
                    buffer.data(),
                    static_cast<DWORD>(buffer.size()),
                    nullptr,
                    0,
                    &bytesReturned,
                    nullptr))
            {
                throwFilesystemError("Failed to create Windows Junction", linkPath);
            }
        }
        catch (...)
        {
            RemoveDirectoryW(Y_flib::FileSystemUtils::pathForIo(linkPath).c_str());
            throw;
        }
    }

} // namespace

namespace Y_flib
{
    std::filesystem::path FileSystemUtils::pathForIo(const std::filesystem::path &path)
    {
        if (path.empty())
        {
            return {};
        }

        const std::wstring original = path.native();
        if (original.starts_with(LR"(\\?\)") || original.starts_with(LR"(\\.\)"))
        {
            return path;
        }

        const std::filesystem::path absolutePath = normalizeAbsolutePath(path);
        const std::wstring native = absolutePath.native();
        if (native.starts_with(LR"(\\)"))
        {
            return std::filesystem::path(
                std::wstring(LR"(\\?\UNC\)") + native.substr(2));
        }

        return std::filesystem::path(std::wstring(LR"(\\?\)") + native);
    }

    FileSystemEntryInfo FileSystemUtils::queryEntry(const std::filesystem::path &path)
    {
        FileSystemEntryInfo info;
        const std::filesystem::path ioPath = pathForIo(path);
        WIN32_FILE_ATTRIBUTE_DATA attributes{};
        if (!GetFileAttributesExW(ioPath.c_str(), GetFileExInfoStandard, &attributes))
        {
            const DWORD error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND ||
                error == ERROR_PATH_NOT_FOUND ||
                error == ERROR_INVALID_NAME)
            {
                return info;
            }

            throwFilesystemError("Failed to query filesystem entry", path, error);
        }

        const bool directoryAttribute =
            (attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        info.exists = true;
        info.isReparsePoint =
            (attributes.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
        // 重解析点与普通目录/文件互斥，扫描器因此不会进入链接目标。
        info.isDirectory = directoryAttribute && !info.isReparsePoint;
        info.isRegularFile = !directoryAttribute && !info.isReparsePoint;
        if (info.isRegularFile)
        {
            info.size =
                (static_cast<Y_flib::FileSize>(attributes.nFileSizeHigh) << 32) |
                static_cast<Y_flib::FileSize>(attributes.nFileSizeLow);
        }
        return info;
    }

    WindowsLinkInfo FileSystemUtils::readLinkForArchive(
        const std::filesystem::path &linkPath)
    {
        const std::filesystem::path normalizedLink = normalizeAbsolutePath(linkPath);
        const FileSystemEntryInfo entry = queryEntry(normalizedLink);
        if (!entry.exists || !entry.isReparsePoint)
        {
            throw std::runtime_error(
                "Entry is not a Windows symbolic link or Junction: " +
                EncodingUtils::pathToUtf8(linkPath));
        }

        WIN32_FILE_ATTRIBUTE_DATA attributes{};
        if (!GetFileAttributesExW(
                pathForIo(normalizedLink).c_str(),
                GetFileExInfoStandard,
                &attributes))
        {
            throwFilesystemError("Failed to query Windows link attributes", linkPath);
        }

        const bool targetIsDirectory =
            (attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        // 在读取处一次性映射为归档 FlagType；目标字符串保持原有语义，
        // 不解析相对/绝对形式，也不检查目标是否存在。
        return readRawLink(normalizedLink, targetIsDirectory);
    }

    void FileSystemUtils::createLink(
        const std::filesystem::path &linkPath,
        const std::filesystem::path &targetPath,
        Y_flib::FlagType linkType)
    {
        if (targetPath.empty())
        {
            throw std::runtime_error("Archived Windows link target is empty");
        }
        if (exists(linkPath))
        {
            throw std::runtime_error(
                "Cannot create Windows link because the destination already exists: " +
                EncodingUtils::pathToUtf8(linkPath));
        }

        if (!linkPath.parent_path().empty())
        {
            createDirectories(linkPath.parent_path());
        }

        switch (linkType)
        {
        case Y_flib::FlagType::SymbolicLinkFile:
            createSymbolicLink(linkPath, targetPath, false);
            break;
        case Y_flib::FlagType::SymbolicLinkDirectory:
            createSymbolicLink(linkPath, targetPath, true);
            break;
        case Y_flib::FlagType::Junction:
        {
            // Junction 的重解析数据要求绝对目标；相对记录只相对链接父目录换算，
            // 不查询目标是否存在，也不继续解析目标中的其他链接。
            const std::filesystem::path junctionTarget =
                targetPath.is_absolute()
                    ? targetPath
                    : linkPath.parent_path() / targetPath;
            createJunction(
                linkPath,
                normalizeAbsolutePath(junctionTarget));
            break;
        }
        default:
            throw std::runtime_error("Invalid Windows link type in archive");
        }
    }

    bool FileSystemUtils::exists(const std::filesystem::path &path)
    {
        return queryEntry(path).exists;
    }

    std::vector<std::filesystem::path> FileSystemUtils::listDirectory(
        const std::filesystem::path &directory)
    {
        std::vector<std::filesystem::path> entries;
        const std::filesystem::path searchPath = pathForIo(directory) / L"*";
        WIN32_FIND_DATAW findData{};
        HANDLE findHandle = FindFirstFileW(searchPath.c_str(), &findData);
        if (findHandle == INVALID_HANDLE_VALUE)
        {
            const DWORD error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND)
            {
                return entries;
            }

            throwFilesystemError("Failed to enumerate directory", directory, error);
        }

        try
        {
            do
            {
                const std::wstring_view name(findData.cFileName);
                if (name != L"." && name != L"..")
                {
                    // 保留普通业务路径，避免把 \\?\ 前缀写入归档。
                    entries.push_back(directory / std::filesystem::path(name));
                }
            } while (FindNextFileW(findHandle, &findData));

            const DWORD error = GetLastError();
            if (error != ERROR_NO_MORE_FILES)
            {
                throwFilesystemError(
                    "Failed while enumerating directory", directory, error);
            }
        }
        catch (...)
        {
            FindClose(findHandle);
            throw;
        }

        FindClose(findHandle);
        return entries;
    }

    bool FileSystemUtils::createDirectory(const std::filesystem::path &path)
    {
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

        throwFilesystemError("Failed to create directory", path, error);
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

        const std::filesystem::path ioPath = pathForIo(path);
        const DWORD attributes = GetFileAttributesW(ioPath.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
            throwFilesystemError("Failed to query entry before removal", path);
        }

        const bool isDirectory =
            (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        std::uintmax_t removed = 0;
        if (isDirectory)
        {
            // 重解析点可能连接到归档树外，只移除链接目录本身。
            if (!info.isReparsePoint)
            {
                for (const std::filesystem::path &child : listDirectory(path))
                {
                    removed += removeAll(child);
                }
            }

            if (!RemoveDirectoryW(ioPath.c_str()))
            {
                throwFilesystemError("Failed to remove directory", path);
            }
        }
        else
        {
            if ((attributes & FILE_ATTRIBUTE_READONLY) != 0)
            {
                SetFileAttributesW(ioPath.c_str(), attributes & ~FILE_ATTRIBUTE_READONLY);
            }

            if (!DeleteFileW(ioPath.c_str()))
            {
                throwFilesystemError("Failed to remove file", path);
            }
        }

        return removed + 1;
    }
} // namespace Y_flib
