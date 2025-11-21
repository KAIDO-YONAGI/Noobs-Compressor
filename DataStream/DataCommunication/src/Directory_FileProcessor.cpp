// Directory_FileProcessor.cpp
#include "../include/Directory_FileProcessor.h"

void Directory_FileProcessor::directory_fileProcessor(std::vector<std::string> &filePathToScan, const std::string &outPutFilePath, const std::string &logicalRoot,std::ofstream &outFile)
{
    Transfer transfer;
    FilePath File;
    MagicNumWriter writer; //创建各个工具类的对象

    fs::path oPath = fs::path(transfer._getPath(outPutFilePath));
    fs::path sPath;

    File.setOutPutFilePath(oPath);

    try
    {
        FileCount_uint length = filePathToScan.size();

        writeLogicalRoot(File, logicalRoot, length,outFile); //写入逻辑根节点的文件数目（默认创建一个根节点，用户可以选择是否命名）
        writeRoot(File, filePathToScan,outFile);             //写入文件根目录

        for (FileCount_uint i = 0; i < length; i++)
        {

            sPath = transfer._getPath(filePathToScan[i]);
            if (!fs::is_regular_file(sPath))
            {
                File.setFilePathToScan(sPath);
                Directory_FileProcessor reader;
                reader.scanFlow(File,outFile);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "directory_fileProcessor()-Error: " << e.what() << std::endl;
    }
    
}

void Directory_FileProcessor::scanFlow(FilePath &File,std::ofstream &outFile)
{

    QueueInterface queue;
    BinaryIO_Reader IO;

    IO.scanner(File, queue,outFile);

    while (!queue.fileQueue.empty())
    {

        FileDetails &details = (queue.fileQueue.front()).first;
        File.setFilePathToScan(details.getFullPath());

        IO.scanner(File, queue,outFile);

        queue.fileQueue.pop();
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

void BinaryIO_Reader::scanner(FilePath &File, QueueInterface &queue,std::ofstream &outFile)
{


    try
    {

        for (auto &entry : fs::directory_iterator(File.getFilePathToScan()))
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
                fullPath); //创建details
            writeBinaryStandard(outFile, details, queue);
        }
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "scanner()-Error: " << e.what() << "\n";
    }

}

void BinaryIO_Reader::writeBinaryStandard(std::ofstream &outFile, FileDetails &details, QueueInterface &queue)
{
    if (details.getIsFile())
    {
        writeFileStandard(outFile, details);
    }
    else
    {
        Directory_FileProcessor reader;
        FileCount_uint countOfThisHeader = reader.countFilesInDirectory(details.getFullPath());

        queue.fileQueue.push({details, countOfThisHeader});
        writeHeaderStandard(outFile, details, countOfThisHeader);
    }
}

void BinaryIO_Reader::writeFileStandard(std::ofstream &outFile, FileDetails &details)
{
    FileNameSize_uint sizeOfName = details.getSizeOfName();
    outFile.write("1", 1);                                //先写文件标
    write_binary_le(outFile, sizeOfName);                 //写入文件名偏移量
    outFile.write(details.getName().c_str(), sizeOfName); //写入文件名
    write_binary_le(outFile, details.getFileSize());      //写入文件大小
    write_binary_le(outFile, FileSize_uint(0));           //预留大小
}

void BinaryIO_Reader::writeHeaderStandard(std::ofstream &outFile, FileDetails &details, FileCount_uint count)
{
    FileNameSize_uint sizeOfName = details.getSizeOfName();
    outFile.write("0", 1);
    write_binary_le(outFile, sizeOfName);
    outFile.write(details.getName().c_str(), sizeOfName);
    write_binary_le(outFile, count); //写入文件数目
}

FileSize_uint BinaryIO_Reader::getFileSize(fs::path &filePathToScan)
{
    try
    {
        return fs::file_size(filePathToScan);
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "getFileSize()-Error: " << e.what() << "\n";
        return 0;
    }
}

void Directory_FileProcessor::writeLogicalRoot(FilePath &File, const std::string &logicalRoot, const FileCount_uint count,std::ofstream &outFile)
{


    FileNameSize_uint rootLength = logicalRoot.size();
    outFile.write("0", 1);
    write_binary_le(outFile, rootLength);
    outFile.write(logicalRoot.c_str(), rootLength);
    write_binary_le(outFile, count); //写文件数
}
void Directory_FileProcessor::writeRoot(FilePath &File, std::vector<std::string> &filePathToScan,std::ofstream &outFile)
{
    Transfer transfer;

    FileCount_uint length = filePathToScan.size();
    for (FileCount_uint i = 0; i < length; i++)
    {

        fs::path sPath = transfer._getPath(filePathToScan[i]);

        if (fs::exists(sPath))
        {
            File.setFilePathToScan(sPath);
        }
        else
            std::cerr << "directory_fileProcessor()-Error:File Not Exist";

        Directory_FileProcessor reader;
        BinaryIO_Reader BIO;
        fs::path rootPath = File.getFilePathToScan(); // 获取根目录

        // 1. 先写入根目录(或文件)自身（手动构造）

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
            BIO.writeHeaderStandard(outFile, rootDetails, count);
        }
        else if (rootDetails.getIsFile())
        {
            BIO.writeFileStandard(outFile, rootDetails);
        }
    }
}