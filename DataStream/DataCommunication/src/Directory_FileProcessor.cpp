// Directory_FileProcessor.cpp
#include "../include/Directory_FileProcessor.h"

void Directory_FileProcessor::directory_fileProcessor(const std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot)
{
    FilePath file; // 创建各个工具类的对象
    BinaryIO_Reader BIO(outFile);
    NumsWriter numWriter(outFile);

    fs::path oPath = fullOutPath;
    fs::path sPath;

    file.setOutPutFilePath(oPath);

    try
    {
        FileCount_uint length = filePathToScan.size();

        // 预留回填偏移量的字节位置
        DirectoryOffsetSize_uint tempOffset = 0; // 初始偏移量
        DirectoryOffsetSize_uint offset = HeaderSize;

        BIO.makeSeparatedStandard(outFile);

        BIO.writeLogicalRoot(logicalRoot, length, tempOffset); // 写入逻辑根节点的子文件数目（默认创建一个根节点，用户可以选择是否命名）
        BIO.writeRoot(file, filePathToScan, tempOffset);       // 写入文件根目录

        for (FileCount_uint i = 0; i < length; i++)
        {

            sPath = transfer.transPath(filePathToScan[i]);
            if (!fs::is_regular_file(sPath))
            {
                file.setFilePathToScan(sPath);
                BIO.binaryIO_Reader(file, queue, tempOffset, offset); // 添加当前目录到队列以启动整个BFS递推
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

    BinaryIO_Reader BIO(outFile);

    while (!queue.fileQueue.empty())
    {
        FileDetails &details = (queue.fileQueue.front()).first;
        file.setFilePathToScan(details.getFullPath());

        BIO.binaryIO_Reader(file, queue, tempOffset, offset);

        queue.fileQueue.pop();
    }
}

void BinaryIO_Reader::binaryIO_Reader(FilePath &file, QueueInterface &queue, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset)
{

    try
    {

        for (auto &entry : fs::directory_iterator(file.getFilePathToScan()))
        {
            bool FileOrDirec;
            if (entry.is_regular_file())
                FileOrDirec = true;
            else if (entry.is_directory())
                FileOrDirec = false;
            else
                continue;//禁用两个基本文件类型之外的文件类型
            std::string name = entry.path().filename().string();
            auto fullPath = entry.path();
            FileNameSize_uint sizeOfName = name.size();

            FileSize_uint fileSize = FileOrDirec ? entry.file_size() : 0;

            FileDetails details(
                name,
                sizeOfName,
                fileSize,
                FileOrDirec,
                fullPath); // 创建details

            writeStorageStandard(details, queue, tempOffset, offset);
        }
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "binaryIO_Reader()-Error: " << e.what() << "\n";
    }
}

void BinaryIO_Reader::writeStorageStandard(FileDetails &details, QueueInterface &queue, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset)
{
    NumsWriter numWriter(outFile);
    if (details.getIsFile()) // 文件对应的处理
    {
        writeFileStandard(details, tempOffset);
    }
    else if (!details.getIsFile()) // 目录对应的处理
    {
        FileCount_uint countOfThisDirectory = countFilesInDirectory(details.getFullPath());

        queue.fileQueue.push({details, countOfThisDirectory}); // 如果是目录则存入其details与其子文件数目的std::pair 到队列中备用
        writeDirectoryStandard(details, countOfThisDirectory, tempOffset);
    }
    if (tempOffset >= BufferSize) // 达到缓冲大小后写入分割标准
    {

        writeSeparatedStandard(tempOffset, offset);
        offset += tempOffset;
        // 写入完毕后将当前相对位置（用多个存储协议偏移量累加维护的tempOffset）加到文件的总偏移量offset上，以维护整个偏移逻辑
        tempOffset = 0; // 相对位置归零

        // 预留下一次回填的位置
        makeSeparatedStandard(outFile);
        offset += SeparatedStandardSize; // 更新offset，保证回填正确。不更新tempOffset，为的是将分割标准的大小排除在外，便于拿到偏移量能不经变换直接操作对应位置的数据
    }
}
void BinaryIO_Reader::writeDirectoryStandard(FileDetails &details, FileCount_uint count, DirectoryOffsetSize_uint &tempOffset)
{
    NumsWriter numWriter(outFile);
    FileNameSize_uint sizeOfName = details.getSizeOfName();

    tempOffset += DirectoryrStandardSize_Basic + sizeOfName;

    outFile.write(HeaderFlag, FlagSize);
    numWriter.writeBinaryNums(sizeOfName);
    outFile.write(details.getName().c_str(), sizeOfName);
    numWriter.writeBinaryNums(count); // 写入文件数目
}
void BinaryIO_Reader::writeFileStandard(FileDetails &details, DirectoryOffsetSize_uint &tempOffset)
{
    NumsWriter numWriter(outFile);
    FileNameSize_uint sizeOfName = details.getSizeOfName();

    tempOffset += FileStandardSize_Basic + sizeOfName;

    outFile.write(FileFlag, FlagSize);                    // 先写文件标
    numWriter.writeBinaryNums(sizeOfName);                // 写入文件名偏移量
    outFile.write(details.getName().c_str(), sizeOfName); // 写入文件名
    numWriter.writeBinaryNums(details.getFileSize());     // 写入文件大小
    numWriter.writeBinaryNums(FileSize_uint(0));          // 预留大小
}

void BinaryIO_Reader::writeSeparatedStandard(DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint offset)
{
    NumsWriter numWriter(outFile);
    Locator locator;

    locator.offsetLocator(outFile, offset + FlagSize);
    numWriter.writeBinaryNums(tempOffset);
    outFile.seekp(0, std::ios::end);
}
void BinaryIO_Reader::makeSeparatedStandard(std::ofstream &outFile)
{
    NumsWriter numWriter(outFile);
    outFile.write(SeparatedFlag, FlagSize);
    numWriter.writeBinaryNums(DirectoryOffsetSize_uint(0));
    numWriter.writeBinaryNums(IvSize_uint(0));
}
void BinaryIO_Reader::writeLogicalRoot(const std::string &logicalRoot, const FileCount_uint count, DirectoryOffsetSize_uint &tempOffset)
{
    FileNameSize_uint sizeOfName = logicalRoot.size();
    NumsWriter numWriter(outFile);
    tempOffset += DirectoryrStandardSize_Basic + sizeOfName;

    outFile.write(LogicalRootFlag, FlagSize);
    numWriter.writeBinaryNums(sizeOfName);
    outFile.write(logicalRoot.c_str(), sizeOfName);
    numWriter.writeBinaryNums(count); // 写文件数
}
void BinaryIO_Reader::writeRoot(FilePath &file, const std::vector<std::string> &filePathToScan, DirectoryOffsetSize_uint &tempOffset)
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
        bool FileOrDirec = fs::is_regular_file(rootPath);
        FileSize_uint fileSize = FileOrDirec ? fs::file_size(rootPath) : 0;

        FileDetails rootDetails(
            rootName,     // 目录名 (如 "Folder")
            rootNameSize, // 名称长度
            fileSize,     // 文件大小(如果是文件)
            FileOrDirec,  // 是否为常规文件
            rootPath      // 完整路径
        );
        BinaryIO_Reader BIO(outFile);
        if (!rootDetails.getIsFile())
        {
            FileCount_uint count = BIO.countFilesInDirectory(rootPath);
            BIO.writeDirectoryStandard(rootDetails, count, tempOffset);
        }
        else if (rootDetails.getIsFile())
        {
            BIO.writeFileStandard(rootDetails, tempOffset);
        }
    }
}
FileCount_uint BinaryIO_Reader::countFilesInDirectory(const fs::path &filePathToScan)
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
FileSize_uint BinaryIO_Reader::getFileSize(const fs::path &filePathToScan)
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
