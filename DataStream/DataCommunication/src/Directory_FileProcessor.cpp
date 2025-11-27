// Directory_FileProcessor.cpp
#include "../include/Directory_FileProcessor.h"

void Directory_FileProcessor::directory_fileProcessor(const std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot, std::ofstream &outFile)
{
    Transfer transfer;
    BinaryIO_Reader BIO;
    FilePath file; // 创建各个工具类的对象

    fs::path oPath = fullOutPath;
    fs::path sPath;

    file.setOutPutFilePath(oPath);

    try
    {
        FileCount_uint length = filePathToScan.size();

        // 预留回填偏移量的字节位置
        DirectoryOffsetSize_uint tempOffset = 0; // 初始偏移量
        DirectoryOffsetSize_uint offset = HeaderSize;

        outFile.write(SeparatedFlag, FlagSize);
        BIO.writeBinary(outFile, DirectoryOffsetSize_uint(0));

        writeLogicalRoot(file, logicalRoot, length, outFile, tempOffset); // 写入逻辑根节点的子文件数目（默认创建一个根节点，用户可以选择是否命名）
        writeRoot(file, filePathToScan, outFile, tempOffset);             // 写入文件根目录

        for (FileCount_uint i = 0; i < length; i++)
        {

            sPath = transfer._getPath(filePathToScan[i]);
            if (!fs::is_regular_file(sPath))
            {
                file.setFilePathToScan(sPath);
                Directory_FileProcessor reader;
                reader.scanFlow(file, outFile, tempOffset, offset);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "directory_fileProcessor()-Error: " << e.what() << std::endl;
    }
}

void Directory_FileProcessor::scanFlow(FilePath &file, std::ofstream &outFile, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset)
{

    QueueInterface queue;
    BinaryIO_Reader BIO;

    BIO.scanner(file, queue, outFile, tempOffset, offset); // 添加当前目录到队列以启动整个BFS递推

    while (!queue.fileQueue.empty())
    {
        FileDetails &details = (queue.fileQueue.front()).first;
        file.setFilePathToScan(details.getFullPath());

        BIO.scanner(file, queue, outFile, tempOffset, offset);

        queue.fileQueue.pop();
    }
}

void BinaryIO_Reader::scanner(FilePath &file, QueueInterface &queue, std::ofstream &outFile, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset)
{

    try
    {

        for (auto &entry : fs::directory_iterator(file.getFilePathToScan()))
        {
            std::string name = entry.path().filename().string();

            auto fullPath = entry.path();
            FileNameSize_uint sizeOfName = name.size();
            bool isRegularFile = entry.is_regular_file();
            FileSize_uint fileSize = isRegularFile ? entry.file_size() : 0;

            FileDetails details(
                name,
                sizeOfName,
                fileSize,
                isRegularFile,
                fullPath); // 创建details

            writeStorageStandard(outFile, details, queue, file, tempOffset, offset);
        }
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "scanner()-Error: " << e.what() << "\n";
    }
}

void BinaryIO_Reader::writeStorageStandard(std::ofstream &outFile, FileDetails &details, QueueInterface &queue, FilePath &file, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset)
{
    FileSize_uint basicSize = 0;
    if (details.getIsFile()) // 文件对应的处理
    {
        basicSize = FileStandardSize_Basic;
        writeFileStandard(outFile, details, tempOffset);
    }
    else if (!details.getIsFile()) // 目录对应的处理
    {
        Directory_FileProcessor reader;
        FileCount_uint countOfThisHeader = reader.countFilesInDirectory(details.getFullPath());

        basicSize = DirectoryrStandardSize_Basic;
        queue.fileQueue.push({details, countOfThisHeader}); // 如果是目录则存入其details与其子文件数目的std::pair 到队列中备用
        writeDirectoryStandard(outFile, details, countOfThisHeader, tempOffset);
    }

    if (tempOffset >= BufferSize) // 达到缓冲大小后写入分割标准
    {

        BinaryIO_Reader BIO;
        writeSeparatedStandard(outFile, file, tempOffset, offset);
        offset += tempOffset;
        // 写入完毕后将当前相对位置（用多个存储协议偏移量累加维护的tempOffset）加到文件的总偏移量offset上，以维护整个偏移逻辑
        tempOffset = 0; // 相对位置归零

        // 预留下一次回填的位置
        outFile.write(SeparatedFlag, FlagSize);
        BIO.writeBinary(outFile, DirectoryOffsetSize_uint(0));

        offset += SeparatedStandardSize; // 更新offset，保证回填正确。不更新tempOffset，为的是将分割标准的大小排除在外，便于拿到偏移量能不经变换直接操作对应位置的数据
    }
}
void BinaryIO_Reader::writeDirectoryStandard(std::ofstream &outFile, FileDetails &details, FileCount_uint count, DirectoryOffsetSize_uint &tempOffset)
{
    BinaryIO_Reader BIO;
    FileNameSize_uint sizeOfName = details.getSizeOfName();

    tempOffset += DirectoryrStandardSize_Basic + sizeOfName;

    outFile.write(HeaderFlag, FlagSize);
    BIO.writeBinary(outFile, sizeOfName);
    outFile.write(details.getName().c_str(), sizeOfName);
    BIO.writeBinary(outFile, count); // 写入文件数目
}
void BinaryIO_Reader::writeFileStandard(std::ofstream &outFile, FileDetails &details, DirectoryOffsetSize_uint &tempOffset)
{
    BinaryIO_Reader BIO;
    FileNameSize_uint sizeOfName = details.getSizeOfName();

    tempOffset += FileStandardSize_Basic + sizeOfName;

    outFile.write(FileFlag, FlagSize);                    // 先写文件标
    BIO.writeBinary(outFile, sizeOfName);                 // 写入文件名偏移量
    outFile.write(details.getName().c_str(), sizeOfName); // 写入文件名
    BIO.writeBinary(outFile, details.getFileSize());      // 写入文件大小
    BIO.writeBinary(outFile, FileSize_uint(0));           // 预留大小
}

void BinaryIO_Reader::writeSeparatedStandard(std::ofstream &outFile, FilePath &file, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint offset)
{
    Locator locator;
    BinaryIO_Reader BIO;
    locator.offsetLocator(outFile, offset + FlagSize);
    BIO.writeBinary(outFile, tempOffset);
    outFile.seekp(0, std::ios::end);
}
void Directory_FileProcessor::writeLogicalRoot(FilePath &file, const std::string &logicalRoot, const FileCount_uint count, std::ofstream &outFile, DirectoryOffsetSize_uint &tempOffset)
{
    BinaryIO_Reader BIO;
    FileNameSize_uint sizeOfName = logicalRoot.size();

    tempOffset += DirectoryrStandardSize_Basic + sizeOfName;

    outFile.write(HeaderFlag, FlagSize);
    BIO.writeBinary(outFile, sizeOfName);
    outFile.write(logicalRoot.c_str(), sizeOfName);
    BIO.writeBinary(outFile, count); // 写文件数
}
void Directory_FileProcessor::writeRoot(FilePath &file, const std::vector<std::string> &filePathToScan, std::ofstream &outFile, DirectoryOffsetSize_uint &tempOffset)
{
    Transfer transfer;
    Directory_FileProcessor reader;
    BinaryIO_Reader BIO; // 创建各个工具类的对象

    FileCount_uint length = filePathToScan.size();
    for (FileCount_uint i = 0; i < length; i++)
    {

        fs::path sPath = transfer._getPath(filePathToScan[i]);

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
        bool isRegularFile = fs::is_regular_file(rootPath);
        FileSize_uint fileSize = isRegularFile ? fs::file_size(rootPath) : 0;

        FileDetails rootDetails(
            rootName,      // 目录名 (如 "Folder")
            rootNameSize,  // 名称长度
            fileSize,      // 文件大小(如果是文件)
            isRegularFile, // 是否为常规文件
            rootPath       // 完整路径
        );

        if (!rootDetails.getIsFile())
        {
            FileCount_uint count = reader.countFilesInDirectory(rootPath);
            BIO.writeDirectoryStandard(outFile, rootDetails, count, tempOffset);
        }
        else if (rootDetails.getIsFile())
        {
            BIO.writeFileStandard(outFile, rootDetails, tempOffset);
        }
    }
}
FileCount_uint Directory_FileProcessor::countFilesInDirectory(const fs::path &filePathToScan)
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
FileSize_uint BinaryIO_Reader::getFileSize(const fs::path &filePathToScan, std::ofstream &outFile)
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
