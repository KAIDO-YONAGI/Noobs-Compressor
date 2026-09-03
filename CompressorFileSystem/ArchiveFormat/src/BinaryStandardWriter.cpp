#include "../include/BinaryStandardWriter.h"
#include "../../Commons/include/FileSystemUtils.h"

namespace Y_flib
{
    void BinaryStandardWriter::binaryStandardWriter(
        const std::filesystem::path &directoryPath,
        EntryQueue &entryQueue,
        DirectoryScanCursor &cursor)
    {
        try
        {
            // 枚举和计数共用同一套长路径元数据，避免“写入数量”和实际条目不一致。
            for (const std::filesystem::path &fullPath :
                 FileSystemUtils::listDirectory(directoryPath))
            {
                std::string name;

                name = EncodingUtils::u8ToString(fullPath.filename().u8string());
                const FileSystemEntryInfo info = FileSystemUtils::queryEntry(fullPath);
                if (!info.exists)
                {
                    throw std::runtime_error(
                        "Filesystem entry disappeared while scanning: " +
                        EncodingUtils::pathToUtf8(fullPath));
                }

                if (info.isReparsePoint)
                {
                    const WindowsLinkInfo linkInfo =
                        FileSystemUtils::readLinkForArchive(fullPath);
                    EntryDetails details(name, 0, false, fullPath);
                    writeLinkStandard(details, linkInfo, cursor);
                    separateBlockIfNeeded(cursor);
                    continue;
                }

                if (!info.isRegularFile && !info.isDirectory)
                {
                    continue;
                }

                EntryDetails details(
                    name,
                    info.isRegularFile ? info.size : 0,
                    info.isRegularFile,
                    fullPath);

                writeStorageStandard(details, entryQueue, cursor);
            }
        }
        catch (std::filesystem::filesystem_error &e)
        {
            throw std::runtime_error(std::string("BinaryStandardWriter encountered a filesystem error: ") + e.what());
        }
    }
    // 识别存储标准并且分发到各个写入函数
    void BinaryStandardWriter::writeStorageStandard(EntryDetails &details, EntryQueue &entryQueue, DirectoryScanCursor &cursor)
    {

        if (details.getIsFile()) // 文件对应的处理
        {
            writeFileStandard(details, cursor);
        }
        else // 链接和不支持的类型已在调用前处理，因此非文件条目这里只可能是目录。
        {
            Y_flib::FileCount countOfThisDirectory = countFilesInDirectory(details.getFullPath());

            entryQueue.push({details, countOfThisDirectory}); // 如果是目录则存入其details与其子文件数目的std::pair 到队列中备用
            writeDirectoryStandard(details, countOfThisDirectory, cursor);
        }
        separateBlockIfNeeded(cursor);
    }
    // 目录标准写入函数
    void BinaryStandardWriter::writeDirectoryStandard(EntryDetails &details, Y_flib::FileCount count, DirectoryScanCursor &cursor)
    {
        Y_flib::FileNameSize sizeOfName = details.getSizeOfName();

        cursor.accountEntry(Y_flib::Constants::DIRECTORY_STANDARD_SIZE_BASIC + sizeOfName);

        standardWriter.writeBinaryStandards(Y_flib::FlagType::Directory, outFile);
        standardWriter.writeBinaryStandards(sizeOfName, outFile);

        standardWriter.writeBinaryStandards(details.getName(), outFile);

        standardWriter.writeBinaryStandards(count, outFile); // 写入文件数目
    }
    // 文件标准写入函数
    void BinaryStandardWriter::writeFileStandard(EntryDetails &details, DirectoryScanCursor &cursor)
    {
        Y_flib::FileNameSize sizeOfName = details.getSizeOfName();

        cursor.accountEntry(Y_flib::Constants::FILE_STANDARD_SIZE_BASIC + sizeOfName);

        standardWriter.writeBinaryStandards(Y_flib::FlagType::File, outFile); // 先写文件标
        standardWriter.writeBinaryStandards(sizeOfName, outFile);             // 写入文件名偏移量

        standardWriter.writeBinaryStandards(details.getName(), outFile); // 写入文件名

        standardWriter.writeBinaryStandards(details.getFileSizeInDetails(), outFile); // 写入文件大小
        standardWriter.writeBinaryStandards(Y_flib::FileSize(0), outFile);            // 预留大小
    }
    // 分割标准写入函数（把当前块累计字节数回填到长度槽位）
    void BinaryStandardWriter::writeSeparatedStandard(DirectoryScanCursor &cursor)
    {
        locator.locateFromBegin(outFile, cursor.lengthSlotPos());
        standardWriter.writeBinaryStandards(cursor.blockBytes, outFile);
        locator.locateFromEnd(outFile, 0);
    }
    // 空分割标准写入函数
    void BinaryStandardWriter::writeBlankSeparatedStandard()
    {
        standardWriter.writeBinaryStandards(Y_flib::FlagType::Separated, outFile);
        standardWriter.writeBinaryStandards(Y_flib::BlockLength(0), outFile);
        standardWriter.writeBinaryStandards(Y_flib::IvSize{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}, outFile);
    }
    /**
     * 将 Windows 符号链接或 Junction 写入归档的目录元数据区。
     *
     * 磁盘布局：
     *   [链接类型][名称长度][目标长度][链接名称][链接记录的目标路径]
     *
     * details 只提供链接名称等通用条目信息，linkInfo 提供从重解析点本身
     * 读取到的最终归档类型和目标路径。FileSystemUtils 已根据重解析标签
     * 和目录属性完成一次性类型判断；本函数只负责序列化，不再重复映射。
     * 本函数不会访问、解析或扫描链接目标，因此目标位于归档外或目标不
     * 存在时也可以正常写入。
     */
    void BinaryStandardWriter::writeLinkStandard(
        EntryDetails &details,
        const WindowsLinkInfo &linkInfo,
        DirectoryScanCursor &cursor)
    {
        Y_flib::FileNameSize sizeOfName = details.getSizeOfName();
        std::string pathStr = EncodingUtils::pathToUtf8(linkInfo.targetPath);
        Y_flib::FileNameSize sizeOfPath = pathStr.size();

        cursor.accountEntry(
            Y_flib::Constants::LINK_STANDARD_SIZE_BASIC + sizeOfName + sizeOfPath);

        standardWriter.writeBinaryStandards(linkInfo.linkType, outFile);

        standardWriter.writeBinaryStandards(sizeOfName, outFile);
        standardWriter.writeBinaryStandards(sizeOfPath, outFile);

        standardWriter.writeBinaryStandards(details.getName(), outFile);
        standardWriter.writeBinaryStandards(pathStr, outFile);
    }

    void BinaryStandardWriter::separateBlockIfNeeded(
        DirectoryScanCursor &cursor)
    {
        if (!cursor.needsSeparation())
        {
            return;
        }

        writeSeparatedStandard(cursor);
        cursor.onBlockSealed();
        writeBlankSeparatedStandard();
        cursor.onSlotReserved();
    }
    void BinaryStandardWriter::writeLogicalRoot(const std::string &logicalRoot, const Y_flib::FileCount count, DirectoryScanCursor &cursor)
    {
        Y_flib::FileNameSize sizeOfName = logicalRoot.size();
        cursor.accountEntry(Y_flib::Constants::DIRECTORY_STANDARD_SIZE_BASIC + sizeOfName);

        standardWriter.writeBinaryStandards(Y_flib::FlagType::LogicalRoot, outFile);
        standardWriter.writeBinaryStandards(sizeOfName, outFile);

        standardWriter.writeBinaryStandards(logicalRoot, outFile); // 写入逻辑根节点名称，属于writeBinaryStandards的字符串参数重载函数

        standardWriter.writeBinaryStandards(count, outFile); // 写文件数
    }
    void BinaryStandardWriter::writeRoot(
        const std::vector<std::string> &filePathToScan,
        DirectoryScanCursor &cursor)
    {
        Y_flib::FileCount num = filePathToScan.size();
        for (Y_flib::FileCount i = 0; i < num; i++)
        {

            std::filesystem::path sPath = EncodingUtils::pathFromUtf8(filePathToScan[i]);

            const FileSystemEntryInfo info = FileSystemUtils::queryEntry(sPath);
            if (!info.exists)
            {
                throw std::runtime_error(
                    "entryProcessor()-Error:file Not Exist: " +
                    EncodingUtils::pathToUtf8(sPath));
            }

            const std::filesystem::path &parentPath = sPath;

            // 先写入根目录(或文件)自身（手动构造）
            // 使用 u8string() 获取 UTF-8 编码的文件名
            std::string rootName = EncodingUtils::u8ToString(parentPath.filename().u8string());
            if (info.isReparsePoint)
            {
                // 链接可作为独立根条目归档，只保存链接自身，不扫描或读取目标。
                const WindowsLinkInfo linkInfo =
                    FileSystemUtils::readLinkForArchive(parentPath);
                EntryDetails rootDetails(
                    rootName, 0, false, parentPath);
                writeLinkStandard(rootDetails, linkInfo, cursor);
                continue;
            }

            bool isFile = info.isRegularFile;
            Y_flib::FileSize fileSize = isFile ? info.size : 0;

            EntryDetails rootDetails(
                rootName,     // 目录名 (如 "Folder")
                fileSize,     // 文件大小(如果是文件)
                isFile,       // 是否为常规文件
                parentPath    // 完整路径
            );
            if (info.isRegularFile)
            {
                writeFileStandard(rootDetails, cursor);
            }
            else if (info.isDirectory)
            {
                Y_flib::FileCount count = countFilesInDirectory(parentPath);
                writeDirectoryStandard(rootDetails, count, cursor);
            }
            else
            {
                throw std::runtime_error(
                    "entryProcessor()-Error:Unsupported file type: " +
                    EncodingUtils::pathToUtf8(parentPath));
            }
        }
    }
    Y_flib::FileCount BinaryStandardWriter::countFilesInDirectory(const std::filesystem::path &filePathToScan)
    {
        try
        {
            Y_flib::FileCount count = 0;
            // 仅统计后续确实会写入归档的三类条目，保持目录子项计数严格一致。
            for (const std::filesystem::path &fullPath :
                 FileSystemUtils::listDirectory(filePathToScan))
            {
                const FileSystemEntryInfo info = FileSystemUtils::queryEntry(fullPath);
                if (!info.exists)
                {
                    throw std::runtime_error(
                        "Filesystem entry disappeared while counting: " +
                        EncodingUtils::pathToUtf8(fullPath));
                }

                if (info.isRegularFile || info.isDirectory || info.isReparsePoint)
                {
                    ++count;
                }
            }
            return count;
        }
        catch (std::filesystem::filesystem_error &e)
        {
            throw std::runtime_error(
                "countFilesInDirectory()-Error: " + std::string(e.what()));
        }
    }
} // namespace Y_flib
