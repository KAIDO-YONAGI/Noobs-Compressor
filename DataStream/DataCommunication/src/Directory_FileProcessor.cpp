// Directory_FileProcessor.cpp
#include "../include/Directory_FileProcessor.h"

void Directory_FileProcessor::directory_fileProcessor(const std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot)
{
    FilePath file; // 创建各个工具类的对象
    BinaryIO_Writter BIO(std::move(outFile));
    NumsWriter numWriter(outFile);

    fs::path oPath = fullOutPath;
    fs::path sPath;

    file.setOutPutFilePath(oPath);

    try
    {
        FileCount_uint length = filePathToScan.size();

        // 预留回填偏移量的字节位置
        DirectoryOffsetSize_uint tempOffset = 0; // 初始偏移量
        DirectoryOffsetSize_uint offset = HEADER_SIZE;

        BIO.writeBlankSeparatedStandard();

        BIO.writeLogicalRoot(logicalRoot, length, tempOffset); // 写入逻辑根节点的子文件数目（默认创建一个根节点，用户可以选择是否命名）
        BIO.writeRoot(file, filePathToScan, tempOffset);       // 写入文件根目录

        for (FileCount_uint i = 0; i < length; i++)
        {

            sPath = transfer.transPath(filePathToScan[i]);
            if (!fs::is_regular_file(sPath))
            {
                file.setFilePathToScan(sPath);
                BIO.binaryIO_Reader(file, directoryQueue, tempOffset, offset); // 添加当前目录到队列以启动整个BFS递推
            }
        }
        scanFlow(file, tempOffset, offset);
    }
    catch (const std::exception &e)
    {
        std::cerr << "directory_fileProcessor()-Error: " << e.what() << std::endl;
    }
}

void Directory_FileProcessor::scanFlow(FilePath &file, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset)
{

    BinaryIO_Writter BIO(std::move(outFile));

    while (!directoryQueue.Directory_FileQueue.empty())
    {
        Directory_FileDetails &details = (directoryQueue.Directory_FileQueue.front()).first;
        file.setFilePathToScan(details.getFullPath());

        BIO.binaryIO_Reader(file, directoryQueue, tempOffset, offset);

        directoryQueue.Directory_FileQueue.pop();
    }
}

void BinaryIO_Writter::binaryIO_Reader(FilePath &file, directoryQueueInterface &directoryQueue, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset)
{

    try
    {

        for (const fs::directory_entry &entry : fs::directory_iterator(file.getFilePathToScan()))
        {
            bool File_Direc = true;
            std::string name;
            fs::path fullPath;
            FileNameSize_uint sizeOfName;
            FileSize_uint fileSize;
            if (entry.is_regular_file())
            {
                File_Direc = true;
                fileSize = entry.file_size();
            }

            else if (entry.is_directory())
            {
                File_Direc = false;
                fileSize = 0;
            }

            else if (entry.is_symlink())
            {
                File_Direc = false;
                fileSize = 1; // 大小为一，仅表示是符号链接
            }
            else
                continue; // 禁用三个基本文件类型之外的文件类型
            name = entry.path().filename().string();
            fullPath = entry.path();
            sizeOfName = name.size();
            Directory_FileDetails details(
                name,
                sizeOfName,
                fileSize,
                File_Direc,
                fullPath); // 创建details

            writeStorageStandard(details, directoryQueue, tempOffset, offset);
        }
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "binaryIO_Reader()-Error: " << e.what() << "\n";
    }
}
// 识别存储标准并且分发到各个写入函数
void BinaryIO_Writter::writeStorageStandard(Directory_FileDetails &details, directoryQueueInterface &directoryQueue, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset)
{
    NumsWriter numWriter(outFile);
    if (details.getIsFile()) // 文件对应的处理
    {
        writeFileStandard(details, tempOffset);
    }
    else if ((!details.getIsFile()) && (details.getFileSize() == 0)) // 目录对应的处理
    {
        FileCount_uint countOfThisDirectory = countFilesInDirectory(details.getFullPath());

        directoryQueue.Directory_FileQueue.push({details, countOfThisDirectory}); // 如果是目录则存入其details与其子文件数目的std::pair 到队列中备用
        writeDirectoryStandard(details, countOfThisDirectory, tempOffset);
    }
    else if ((!details.getIsFile()) && (details.getFileSize() == 1))
    {
        writeSymbolLinkStandard(details, tempOffset);
    }
    if (tempOffset >= BUFFER_SIZE) // 达到缓冲大小后写入分割标准
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
void BinaryIO_Writter::writeDirectoryStandard(Directory_FileDetails &details, FileCount_uint count, DirectoryOffsetSize_uint &tempOffset)
{
    NumsWriter numWriter(outFile);
    FileNameSize_uint sizeOfName = details.getSizeOfName();

    tempOffset += DIRECTORY_STANDARD_SIZE_BASIC + sizeOfName;

    outFile.write(HEADER_FLAG, FLAG_SIZE);
    numWriter.writeBinaryNums(sizeOfName);
    outFile.write(details.getName().c_str(), sizeOfName);
    numWriter.writeBinaryNums(count); // 写入文件数目
}
// 文件标准写入函数
void BinaryIO_Writter::writeFileStandard(Directory_FileDetails &details, DirectoryOffsetSize_uint &tempOffset)
{
    NumsWriter numWriter(outFile);
    FileNameSize_uint sizeOfName = details.getSizeOfName();

    tempOffset += FILE_STANDARD_SIZE_BASIC + sizeOfName;

    outFile.write(FILE_FLAG, FLAG_SIZE);                    // 先写文件标
    numWriter.writeBinaryNums(sizeOfName);                // 写入文件名偏移量
    outFile.write(details.getName().c_str(), sizeOfName); // 写入文件名
    numWriter.writeBinaryNums(details.getFileSize());     // 写入文件大小
    numWriter.writeBinaryNums(FileSize_uint(0));          // 预留大小
}
// 分割标准写入函数（回填）
void BinaryIO_Writter::writeSeparatedStandard(DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint offset)
{
    NumsWriter numWriter(outFile);
    Locator locator;

    locator.offsetLocator(outFile, offset + FLAG_SIZE);
    numWriter.writeBinaryNums(tempOffset);
    outFile.seekp(0, std::ios::end);
}
// 空分割标准写入函数
void BinaryIO_Writter::writeBlankSeparatedStandard()
{
    NumsWriter numWriter(outFile);
    outFile.write(SEPARATED_FLAG, FLAG_SIZE);
    numWriter.writeBinaryNums(DirectoryOffsetSize_uint(0));
    numWriter.writeBinaryNums(IvSize_uint(0));
}
//由于加密模式iv包含在数据区内，直接写入不含iv部分的空分割标准
void BinaryIO_Writter::writeBlankSeparatedStandardForEncryption(std::fstream &File){
    NumsWriter numWriter(outFile);
    File.write(SEPARATED_FLAG, FLAG_SIZE);
    numWriter.writeBinaryNums(DirectoryOffsetSize_uint(0),File);
}
// 符号链接标准写入函数
void BinaryIO_Writter::writeSymbolLinkStandard(Directory_FileDetails &details, DirectoryOffsetSize_uint &tempOffset)
{
    NumsWriter numWriter(outFile);
    FileNameSize_uint sizeOfName = details.getSizeOfName();
    FileNameSize_uint sizeOfPath = details.getFullPath().string().size();

    tempOffset += SYMBOL_LINK_STANDARD_SIZE_BASIC + sizeOfName + sizeOfPath;

    outFile.write(SYMBOL_LINK_FLAG, FLAG_SIZE);

    numWriter.writeBinaryNums(sizeOfName);
    numWriter.writeBinaryNums(sizeOfPath);

    outFile.write(details.getName().c_str(), sizeOfName);
    outFile.write(details.getFullPath().string().c_str(), sizeOfPath);
}
void BinaryIO_Writter::writeLogicalRoot(const std::string &logicalRoot, const FileCount_uint count, DirectoryOffsetSize_uint &tempOffset)
{
    FileNameSize_uint sizeOfName = logicalRoot.size();
    NumsWriter numWriter(outFile);
    tempOffset += DIRECTORY_STANDARD_SIZE_BASIC + sizeOfName;

    outFile.write(LOGICAL_ROOT_FLAG, FLAG_SIZE);
    numWriter.writeBinaryNums(sizeOfName);
    outFile.write(logicalRoot.c_str(), sizeOfName);
    numWriter.writeBinaryNums(count); // 写文件数
}
void BinaryIO_Writter::writeRoot(FilePath &file, const std::vector<std::string> &filePathToScan, DirectoryOffsetSize_uint &tempOffset)
{
    FileCount_uint length = filePathToScan.size();
    for (FileCount_uint i = 0; i < length; i++)
    {

        fs::path sPath = transfer.transPath(filePathToScan[i]);

        if (fs::exists(sPath))
        {
            file.setFilePathToScan(sPath);
        }
        else
        {
            throw("directory_fileProcessor()-Error:file Not Exist: " + sPath.string() + "\n");
        }

        fs::path rootPath = file.getFilePathToScan(); // 获取根目录

        // 先写入根目录(或文件)自身（手动构造）

        std::string rootName = rootPath.filename().string();
        FileNameSize_uint rootNameSize = rootName.size();
        bool File_Direc = fs::is_regular_file(rootPath);
        FileSize_uint fileSize = File_Direc ? fs::file_size(rootPath) : 0;

        Directory_FileDetails rootDetails(
            rootName,     // 目录名 (如 "Folder")
            rootNameSize, // 名称长度
            fileSize,     // 文件大小(如果是文件)
            File_Direc,  // 是否为常规文件
            rootPath      // 完整路径
        );
        BinaryIO_Writter BIO(std::move(outFile));
        const fs::directory_entry entry(rootPath);
        if (entry.is_regular_file())
        {
            BIO.writeFileStandard(rootDetails, tempOffset);
        }
        else if (entry.is_directory())
        {
            FileCount_uint count = BIO.countFilesInDirectory(rootPath);
            BIO.writeDirectoryStandard(rootDetails, count, tempOffset);
        }
        else if (entry.is_symlink())
        {
            rootDetails.setFileSize(1);
            BIO.writeSymbolLinkStandard(rootDetails, tempOffset);
        }
    }
}
FileCount_uint BinaryIO_Writter::countFilesInDirectory(const fs::path &filePathToScan)
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
FileSize_uint BinaryIO_Writter::getFileSize(const fs::path &filePathToScan)
{
    try
    {
        outFile.flush();

        return fs::file_size(filePathToScan);
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "getFileSize()-Error: " << e.what() << "\n";
        return 0;
    }
}
