// HeaderReader.cpp
#include "../include/HeaderReader.h"

void HeaderReader::headerReader(std::vector<std::string> &filePathToScan, std::string &outPutFilePath, std::string &logicalRoot)
{

    fs::path oPath = fs::path(_getPath(outPutFilePath));
    fs::path sPath;

    FilePath File;
    File.setOutPutFilePath(oPath);

    if (fs::exists(File.getOutPutFilePath()))
    {
        std::cerr << "headerReader()-Error_fileIsExist\n"
                  << "Try to clear:" << File.getOutPutFilePath()
                  << "\n";
        return;
    }

    appendMagicStatic(File.getOutPutFilePath());

    try
    {

        FileCount_Int length = filePathToScan.size();

        writeLogicalRoot(File, logicalRoot, length); //写入逻辑根节点的文件数目（默认创建一个根节点，用户可以选择是否命名）
        writeRoot(File, filePathToScan);             //写入文件根目录

        for (int i = 0; i < length; i++)
        {

            sPath = _getPath(filePathToScan[i]);
            if (!fs::is_regular_file(sPath))
            {
                File.setFilePathToScan(sPath);
                HeaderReader reader;
                reader.scanFlow(File);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "headerReader()-Error: " << e.what() << std::endl;
    }
    appendMagicStatic(File.getOutPutFilePath());
}

void HeaderReader::scanFlow(FilePath &File)
{

    QueueInterface queue;
    BinaryIO_Reader IO;

    IO.scanner(File, queue);

    while (!queue.fileQueue.empty())
    {

        FileDetails &details = (queue.fileQueue.front()).first;
        File.setFilePathToScan(details.getFullPath());

        IO.scanner(File, queue);

        queue.fileQueue.pop();
    }
}

FileCount_Int HeaderReader::countFilesInDirectory(fs::path &filePathToScan)
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

void BinaryIO_Reader::scanner(FilePath &File, QueueInterface &queue)
{

    std::ofstream outFile(File.getOutPutFilePath(), std::ios::binary | std::ios::app);
    if (!outFile)
    {
        std::cerr << "scanner()-Error_failToOpenFile:" << File.getFilePathToScan() << "\n";
        return;
    }

    try
    {

        for (auto &entry : fs::directory_iterator(File.getFilePathToScan()))
        {
            std::string name = entry.path().filename().string();

            auto fullPath = entry.path();
            FileNameSize_Int sizeOfName = name.size();
            bool is_File = entry.is_regular_file();
            FileSize_Int fileSize = is_File ? entry.file_size() : 0;

            FileDetails details(name, sizeOfName, fileSize, is_File, fullPath); //创建details
            writeBinaryStandard(outFile, details, queue);
        }
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "scanner()-Error: " << e.what() << "\n";
    }

    outFile.close();
}

void BinaryIO_Reader::writeBinaryStandard(std::ofstream &outFile, FileDetails &details, QueueInterface &queue)
{
    if (details.getIsFile())
    {
        writeFileStandard(outFile, details);
    }
    else
    {
        HeaderReader reader;
        FileCount_Int countOfThisHeader = reader.countFilesInDirectory(details.getFullPath());
        if (countOfThisHeader >= 0)
        {
            queue.fileQueue.push({details, countOfThisHeader});
            writeHeaderStandard(outFile, details, countOfThisHeader);
        }
    }
}

void BinaryIO_Reader::writeFileStandard(std::ofstream &outFile, FileDetails &details)
{
    FileNameSize_Int sizeOfName = details.getSizeOfName();
    outFile.write("1", 1);                                //先写文件标
    write_binary_le(outFile, sizeOfName);                 //写入文件名偏移量
    outFile.write(details.getName().c_str(), sizeOfName); //写入文件名
    write_binary_le(outFile, details.getFileSize());      //写入文件大小
    write_binary_le(outFile, FileSize_Int(0));            //预留大小
}

void BinaryIO_Reader::writeHeaderStandard(std::ofstream &outFile, FileDetails &details, FileCount_Int count)
{
    FileNameSize_Int sizeOfName = details.getSizeOfName();
    outFile.write("0", 1);
    write_binary_le(outFile, sizeOfName);
    outFile.write(details.getName().c_str(), sizeOfName);
    write_binary_le(outFile, count); //写入文件数目
}

FileSize_Int BinaryIO_Reader::getFileSize(fs::path &filePathToScan)
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

void HeaderReader::appendMagicStatic(fs::path &outputFilePath)
{
    std::ofstream outFile(outputFilePath, std::ios::binary | std::ios::app);
    if (!outFile)
    {
        std::cerr << "appendMagicStatic-Error_failToOpenFile: " << outputFilePath << "\n";
        return;
    }

    write_binary_le(outFile, MagicNum);
    outFile.close();
}

void HeaderReader::writeLogicalRoot(FilePath &File, std::string &logicalRoot, FileCount_Int count)
{

    std::ofstream outFile(File.getOutPutFilePath(), std::ios::binary | std::ios::app);

    if (!outFile)
    {
        std::cerr << "Error"
                  << "\n";
        return;
    }

    FileNameSize_Int rootLength = logicalRoot.size();
    outFile.write("0", 1);
    write_binary_le(outFile, rootLength);
    outFile.write(logicalRoot.c_str(), rootLength);
    write_binary_le(outFile, count); //写文件数
    outFile.close();
}
void HeaderReader::writeRoot(FilePath &File, std::vector<std::string> &filePathToScan)
{
    FileCount_Int length = filePathToScan.size();
    for (int i = 0; i < length; i++)
    {

        fs::path sPath = _getPath(filePathToScan[i]);

        if (fs::exists(sPath))
        {
            File.setFilePathToScan(sPath);
        }
        else
            std::cerr << "headerReader()-Error:File Not Exist";

        HeaderReader reader;
        BinaryIO_Reader BIO;
        fs::path rootPath = File.getFilePathToScan(); // 获取根目录

        // 1. 先写入根目录(或文件)自身（手动构造）
        FileDetails rootDetails(
            rootPath.filename().string(), // 获取目录名 (如 "Folder")
            rootPath.filename().string().size(),
            fs::is_regular_file(rootPath) ? fs::file_size(rootPath) : 0,
            fs::is_regular_file(rootPath),
            rootPath // 完整路径
        );
        std::ofstream outFile(File.getOutPutFilePath(), std::ios::binary | std::ios::app);
        if (!rootDetails.getIsFile())
        {
            FileCount_Int count = reader.countFilesInDirectory(rootPath);
            BIO.writeHeaderStandard(outFile, rootDetails, count);
        }
        else if (rootDetails.getIsFile())
        {
            BIO.writeFileStandard(outFile, rootDetails);
        }
        outFile.close();
    }
}