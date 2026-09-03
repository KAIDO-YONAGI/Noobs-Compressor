#include "../include/BinaryStandardWriter.h"
#include "../../Commons/include/FileSystemUtils.h"

namespace Y_flib
{
    void BinaryStandardWriter::binaryStandardWriter(FilePath &file, EntryQueue &entryQueue, DirectoryScanCursor &cursor)
    {
        try
        {
            const std::filesystem::path directoryPath = file.getFilePathToScan();
            // 枚举和计数共用同一套长路径元数据，避免“写入数量”和实际条目不一致。
            for (const std::filesystem::path &fullPath :
                 FileSystemUtils::listDirectory(directoryPath))
            {
                bool isFile = true;
                std::string name;
                Y_flib::FileNameSize sizeOfName;
                Y_flib::FileSize fileSize;

                name = EncodingUtils::u8ToString(fullPath.filename().u8string());
                const FileSystemEntryInfo info = FileSystemUtils::queryEntry(fullPath);
                if (!info.exists)
                {
                    throw std::runtime_error(
                        "Filesystem entry disappeared while scanning: " +
                        EncodingUtils::pathToUtf8(fullPath));
                }

                if (info.isRegularFile)
                {
                    isFile = true;
                    fileSize = info.size;
                }

                else if (info.isDirectory)
                {
                    isFile = false;
                    fileSize = 0;
                }

                else if (info.isSymbolicLink)
                {
                    isFile = false;
                    fileSize = 1; // 大小为一的文件夹，仅表示是符号链接
                }
                else
                    continue; // 禁用三个基本文件类型之外的文件类型

                sizeOfName = name.size();
                EntryDetails details(
                    name,
                    sizeOfName,
                    fileSize,
                    isFile,
                    fullPath); // 创建details

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
        else if ((!details.getIsFile()) && (details.getFileSizeInDetails() == 0)) // 目录对应的处理
        {
            Y_flib::FileCount countOfThisDirectory = countFilesInDirectory(details.getFullPath());

            entryQueue.push({details, countOfThisDirectory}); // 如果是目录则存入其details与其子文件数目的std::pair 到队列中备用
            writeDirectoryStandard(details, countOfThisDirectory, cursor);
        }
        else if ((!details.getIsFile()) && (details.getFileSizeInDetails() == 1))
        {
            writeSymbolLinkStandard(details, cursor);
        }
        if (cursor.needsSeparation()) // 达到缓冲大小后：回填块长度并预留下一块
        {
            writeSeparatedStandard(cursor); // 把累计的块字节数回填到当前分割标准的长度槽位
            cursor.onBlockSealed();         // 游标跳过块体，块内累计清零
            writeBlankSeparatedStandard();  // 预留下一次回填的位置
            cursor.onSlotReserved();        // 游标跳过槽位本身（槽位大小不计入 blockBytes，保证槽位可直接定位）
        }
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
    // 由于加密模式iv包含在数据区内，直接写入不含iv部分的空分割标准
    void BinaryStandardWriter::writeBlankSeparatedStandardForEncryption(std::fstream &File)
    {
        standardWriter.writeBinaryStandards(Y_flib::FlagType::Separated, File);
        standardWriter.writeBinaryStandards(Y_flib::BlockLength(0), File);
    }
    // 符号链接标准写入函数
    void BinaryStandardWriter::writeSymbolLinkStandard(EntryDetails &details, DirectoryScanCursor &cursor)
    {
        // 使用 u8string() 获取 UTF-8 编码，确保中文路径正确
        Y_flib::FileNameSize sizeOfName = details.getSizeOfName();
        std::string pathStr = EncodingUtils::pathToUtf8(details.getFullPath());
        Y_flib::FileNameSize sizeOfPath = pathStr.size();

        cursor.accountEntry(Y_flib::Constants::SYMBOL_LINK_STANDARD_SIZE_BASIC + sizeOfName + sizeOfPath);

        standardWriter.writeBinaryStandards(Y_flib::FlagType::SymbolLink, outFile);

        standardWriter.writeBinaryStandards(sizeOfName, outFile);
        standardWriter.writeBinaryStandards(sizeOfPath, outFile);

        standardWriter.writeBinaryStandards(details.getName(), outFile);
        standardWriter.writeBinaryStandards(pathStr, outFile);
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
    void BinaryStandardWriter::writeRoot(FilePath &file, const std::vector<std::string> &filePathToScan, DirectoryScanCursor &cursor)
    {
        Y_flib::FileCount num = filePathToScan.size();
        for (Y_flib::FileCount i = 0; i < num; i++)
        {

            std::filesystem::path sPath = EncodingUtils::pathFromUtf8(filePathToScan[i]);

            const FileSystemEntryInfo info = FileSystemUtils::queryEntry(sPath);
            if (info.exists)
            {
                file.setFilePathToScan(sPath);
            }
            else
            {
                throw std::runtime_error(
                    "entryProcessor()-Error:file Not Exist: " +
                    EncodingUtils::pathToUtf8(sPath));
            }

            std::filesystem::path parentPath = file.getFilePathToScan(); // 获取根目录

            // 先写入根目录(或文件)自身（手动构造）
            // 使用 u8string() 获取 UTF-8 编码的文件名
            std::string rootName = EncodingUtils::u8ToString(parentPath.filename().u8string());
            Y_flib::FileNameSize rootNameSize = rootName.size();
            bool isFile = info.isRegularFile;
            Y_flib::FileSize fileSize = isFile ? info.size : 0;

            EntryDetails rootDetails(
                rootName,     // 目录名 (如 "Folder")
                rootNameSize, // 名称长度
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
            else if (info.isSymbolicLink)
            {
                rootDetails.setFileSize(1); // 利用大小区分符号链接
                writeSymbolLinkStandard(rootDetails, cursor);
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

                if (info.isRegularFile || info.isDirectory || info.isSymbolicLink)
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
    Y_flib::FileSize BinaryStandardWriter::getFileSize(const std::filesystem::path &filePathToScan)
    {
        try
        {
            return locator.getFileSize(filePathToScan, outFile);
        }
        catch (std::filesystem::filesystem_error &e)
        {
            std::cerr << "getFileSize()-Error: " << e.what() << "\n";
            return 0;
        }
    }
} // namespace Y_flib
