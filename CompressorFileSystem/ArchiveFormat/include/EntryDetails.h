// EntryDetails.h
#pragma once
#include "FileLibrary.h"
#include <filesystem>
/* EntryDetails - 文件/目录的元数据容器
 *
 * 功能:
 *   存储单个文件或目录的基本属性
 *   包括名称、大小、类型和完整路径
 *   用于目录遍历和二进制序列化过程中传递信息
 */
namespace Y_flib
{
    class EntryDetails
    {
    private:
        std::string name;
        Y_flib::FileSize fileSize;
        bool isFile;
        std::filesystem::path fullPath;

    public:
        /* 名称长度始终由 name 派生，避免同一信息保存两份后发生不一致。 */
        EntryDetails(std::string name, Y_flib::FileSize fileSize, bool isFile, std::filesystem::path fullPath)
            : name(std::move(name)), fileSize(fileSize), isFile(isFile), fullPath(std::move(fullPath)) {}

        /* 获取文件/目录名称 */
        const std::string &getName() const { return name; }

        /* 获取完整路径 */
        const std::filesystem::path &getFullPath() const { return fullPath; }

        /* name 保存归档使用的 UTF-8 字节，size() 就是磁盘中应写入的名称长度。 */
        Y_flib::FileNameSize getSizeOfName() const
        {
            return static_cast<Y_flib::FileNameSize>(name.size());
        }

        /* 获取文件大小 */
        Y_flib::FileSize getFileSizeInDetails() const { return fileSize; }

        /* 检查是否为文件（false表示目录） */
        bool getIsFile() const { return isFile; }

    };
} // namespace Y_flib
