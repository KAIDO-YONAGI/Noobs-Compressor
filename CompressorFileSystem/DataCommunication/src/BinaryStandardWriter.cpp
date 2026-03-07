#include "../include/BinaryStandardWriter.h"
namespace fs = std::filesystem;

void BinaryStandardWriter::binaryStandardWriter(FilePath &file, EntryQueue &entryQueue, Y_flib::DirectoryOffsetSize &tempOffset, Y_flib::DirectoryOffsetSize &offset)
{
    try
    {
        for (const fs::directory_entry &entry : fs::directory_iterator(file.getFilePathToScan()))
        {
            bool isFile = true;
            std::string name;
            fs::path fullPath;
            Y_flib::FileNameSize sizeOfName;
            Y_flib::FileSize fileSize;
            if (entry.is_regular_file())
            {
                isFile = true;
                fileSize = entry.file_size();
            }

            else if (entry.is_directory())
            {
                isFile = false;
                fileSize = 0;
            }

            else if (entry.is_symlink())
            {
                isFile = false;
                fileSize = 1; // 大小为一，仅表示是符号链接
            }
            else
                continue; // 禁用三个基本文件类型之外的文件类型
            name = entry.path().filename().string();
            fullPath = entry.path();
            sizeOfName = name.size();
            EntryDetails details(
                name,
                sizeOfName,
                fileSize,
                isFile,
                fullPath); // 创建details

            writeStorageStandard(details, entryQueue, tempOffset, offset);
        }
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "binaryStandardWriter()-Error: " << e.what() << "\n";
    }
}
// 识别存储标准并且分发到各个写入函数
void BinaryStandardWriter::writeStorageStandard(EntryDetails &details, EntryQueue &entryQueue, Y_flib::DirectoryOffsetSize &tempOffset, Y_flib::DirectoryOffsetSize &offset)
{

    if (details.getIsFile()) // 文件对应的处理
    {
        writeFileStandard(details, tempOffset);
    }
    else if ((!details.getIsFile()) && (details.getFileSizeInDetails() == 0)) // 目录对应的处理
    {
        Y_flib::FileCount countOfThisDirectory = countFilesInDirectory(details.getFullPath());

        entryQueue.push({details, countOfThisDirectory}); // 如果是目录则存入其details与其子文件数目的std::pair 到队列中备用
        writeDirectoryStandard(details, countOfThisDirectory, tempOffset);
    }
    else if ((!details.getIsFile()) && (details.getFileSizeInDetails() == 1))
    {
        writeSymbolLinkStandard(details, tempOffset);
    }
    if (tempOffset >= DIRECTORY_BUFFER_SIZE) // 达到缓冲大小后写入分割标准
    {

        writeSeparatedStandard(tempOffset, offset);
        offset += tempOffset;
        // 写入完毕后将当前相对位置（用多个存储标准偏移量累加维护的tempOffset）加到文件的总偏移量offset上，以维护整个偏移逻辑
        tempOffset = 0; // 相对位置归零

        // 预留下一次回填的位置
        writeBlankSeparatedStandard();
        offset += SEPARATED_STANDARD_SIZE; // 更新offset，保证回填正确。不更新tempOffset，为的是将分割标准的大小排除在外，便于拿到偏移量能不经变换直接操作对应位置的数据
    }
}
// 目录标准写入函数
void BinaryStandardWriter::writeDirectoryStandard(EntryDetails &details, Y_flib::FileCount count, Y_flib::DirectoryOffsetSize &tempOffset)
{
    Y_flib::FileNameSize sizeOfName = details.getSizeOfName();

    tempOffset += DIRECTORY_STANDARD_SIZE_BASIC + sizeOfName;

    standardWriter.writeBinaryStandards(FlagType::Directory, outFile);
    standardWriter.writeBinaryStandards(sizeOfName, outFile);

    standardWriter.writeBinaryStandards(details.getName(), outFile);

    standardWriter.writeBinaryStandards(count, outFile); // 写入文件数目
}
// 文件标准写入函数
void BinaryStandardWriter::writeFileStandard(EntryDetails &details, Y_flib::DirectoryOffsetSize &tempOffset)
{
    Y_flib::FileNameSize sizeOfName = details.getSizeOfName();

    tempOffset += FILE_STANDARD_SIZE_BASIC + sizeOfName;

    standardWriter.writeBinaryStandards(FlagType::File, outFile);  // 先写文件标
    standardWriter.writeBinaryStandards(sizeOfName, outFile); // 写入文件名偏移量

    standardWriter.writeBinaryStandards(details.getName(), outFile); // 写入文件名

    standardWriter.writeBinaryStandards(details.getFileSizeInDetails(), outFile); // 写入文件大小
    standardWriter.writeBinaryStandards(Y_flib::FileSize(0), outFile);            // 预留大小
}
// 分割标准写入函数（回填）
void BinaryStandardWriter::writeSeparatedStandard(Y_flib::DirectoryOffsetSize &tempOffset, Y_flib::DirectoryOffsetSize offset)
{
    locator.locateFromBegin(outFile, offset + FLAG_SIZE);
    standardWriter.writeBinaryStandards(tempOffset, outFile);
    locator.locateFromEnd(outFile, 0);
}
// 空分割标准写入函数
void BinaryStandardWriter::writeBlankSeparatedStandard()
{
    standardWriter.writeBinaryStandards(FlagType::Separated, outFile);
    standardWriter.writeBinaryStandards(Y_flib::DirectoryOffsetSize(0), outFile);
    standardWriter.writeBinaryStandards(Y_flib::IvSize(0), outFile);
}
// 由于加密模式iv包含在数据区内，直接写入不含iv部分的空分割标准
void BinaryStandardWriter::writeBlankSeparatedStandardForEncryption(std::fstream &File)
{
    standardWriter.writeBinaryStandards(FlagType::Separated, File);
    standardWriter.writeBinaryStandards(Y_flib::DirectoryOffsetSize(0), File);
}
// 符号链接标准写入函数
void BinaryStandardWriter::writeSymbolLinkStandard(EntryDetails &details, Y_flib::DirectoryOffsetSize &tempOffset)
{
    Y_flib::FileNameSize sizeOfName = details.getSizeOfName();
    Y_flib::FileNameSize sizeOfPath = details.getFullPath().string().size();

    tempOffset += SYMBOL_LINK_STANDARD_SIZE_BASIC + sizeOfName + sizeOfPath;

    standardWriter.writeBinaryStandards(FlagType::SymbolLink, outFile);

    standardWriter.writeBinaryStandards(sizeOfName, outFile);
    standardWriter.writeBinaryStandards(sizeOfPath, outFile);

    standardWriter.writeBinaryStandards(details.getName(), outFile);
    standardWriter.writeBinaryStandards(details.getFullPath().string(), outFile);
}
void BinaryStandardWriter::writeLogicalRoot(const std::string &logicalRoot, const Y_flib::FileCount count, Y_flib::DirectoryOffsetSize &tempOffset)
{
    Y_flib::FileNameSize sizeOfName = logicalRoot.size();
    tempOffset += DIRECTORY_STANDARD_SIZE_BASIC + sizeOfName;

    standardWriter.writeBinaryStandards(FlagType::LogicalRoot, outFile);
    standardWriter.writeBinaryStandards(sizeOfName, outFile);

    standardWriter.writeBinaryStandards(logicalRoot, outFile); // 写入逻辑根节点名称，属于writeBinaryStandards的字符串参数重载函数

    standardWriter.writeBinaryStandards(count, outFile); // 写文件数
}
void BinaryStandardWriter::writeRoot(FilePath &file, const std::vector<std::string> &filePathToScan, Y_flib::DirectoryOffsetSize &tempOffset)
{
    Y_flib::FileCount num = filePathToScan.size();
    for (Y_flib::FileCount i = 0; i < num; i++)
    {

        fs::path sPath = transfer.transPath(filePathToScan[i]);

        if (fs::exists(sPath))
        {
            file.setFilePathToScan(sPath);
        }
        else
        {
            throw("entryProcessor()-Error:file Not Exist: " + sPath.string() + "\n");
        }

        fs::path parentPath = file.getFilePathToScan(); // 获取根目录

        // 先写入根目录(或文件)自身（手动构造）

        std::string rootName = parentPath.filename().string();
        Y_flib::FileNameSize rootNameSize = rootName.size();
        bool isFile = fs::is_regular_file(parentPath);
        Y_flib::FileSize fileSize = isFile ? fs::file_size(parentPath) : 0;

        EntryDetails rootDetails(
            rootName,     // 目录名 (如 "Folder")
            rootNameSize, // 名称长度
            fileSize,     // 文件大小(如果是文件)
            isFile,       // 是否为常规文件
            parentPath    // 完整路径
        );
        const fs::directory_entry entry(parentPath);
        if (entry.is_regular_file())
        {
            writeFileStandard(rootDetails, tempOffset);
        }
        else if (entry.is_directory())
        {
            Y_flib::FileCount count = countFilesInDirectory(parentPath);
            writeDirectoryStandard(rootDetails, count, tempOffset);
        }
        else if (entry.is_symlink())
        {
            rootDetails.setFileSize(1); // 利用大小区分符号链接
            writeSymbolLinkStandard(rootDetails, tempOffset);
        }
    }
}
Y_flib::FileCount BinaryStandardWriter::countFilesInDirectory(const fs::path &filePathToScan)
{
    try
    {
        return std::distance(fs::directory_iterator(filePathToScan), fs::directory_iterator{});
    }
    catch (fs::filesystem_error &e)
    {
        throw("countFilesInDirectory()-Error: " + std::string(e.what()) + "\n");
    }
}
Y_flib::FileSize BinaryStandardWriter::getFileSize(const fs::path &filePathToScan)
{
    try
    {
        return locator.getFileSize(filePathToScan, outFile);
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "getFileSize()-Error: " << e.what() << "\n";
        return 0;
    }
}
