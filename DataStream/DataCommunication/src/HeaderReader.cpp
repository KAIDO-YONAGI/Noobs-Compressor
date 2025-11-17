// HeaderReader.cpp
#include "../include/HeaderReader.h"

void BinaryIO_Reader::scanner(FilePath &File, QueueInterface &queue)
{

    std::ofstream outfile(File.getOutPutFilePath(), std::ios::binary | std::ios::app);
    if (!outfile)
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
            writeBinaryStandard(outfile, details, queue);
        }
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "scanner()-Error: " << e.what() << "\n";
    }

    outfile.close();
}

void BinaryIO_Reader::writeBinaryStandard(std::ofstream &outfile, FileDetails &details, QueueInterface &queue)
{
    if (details.getIsFile())
    {
        writeFileStandard(outfile, details);
    }
    else
    {
        HeaderReader reader;
        FileCount_Int countOfThisHeader = reader.countFilesInDirectory(details.getFullPath());
        if (countOfThisHeader >= 0)
        {

            queue.fileQueue.push({details, countOfThisHeader});
            writeHeaderStandard(outfile, details, countOfThisHeader);
        }
    }
}

void BinaryIO_Reader::writeFileStandard(std::ofstream &outfile, FileDetails &details)
{
    FileNameSize_Int SizeOfName = details.getSizeOfName();
    outfile.write("1", 1);                                //先写文件标
    write_binary_le(outfile, SizeOfName);                 //写入文件名偏移量
    outfile.write(details.getName().c_str(), SizeOfName); //写入文件名
    write_binary_le(outfile, details.getFileSize());      //写入文件大小
    write_binary_le(outfile, FileSize_Int(0));            //预留大小
}

void BinaryIO_Reader::writeHeaderStandard(std::ofstream &outfile, FileDetails &details, FileCount_Int count)
{
    FileNameSize_Int SizeOfName = details.getSizeOfName();
    outfile.write("0", 1);
    write_binary_le(outfile, SizeOfName);
    outfile.write(details.getName().c_str(), SizeOfName);
    write_binary_le(outfile, count); //写入文件数目
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

void HeaderReader::writeRoot(FilePath &File)
{

    std::ofstream outFile(File.getOutPutFilePath(), std::ios::binary | std::ios::app);
    write_binary_le(outFile, countFilesInDirectory(File.getFilePathToScan()));
    outFile.close();
}
void HeaderReader::scanFlow(FilePath &File)
{
    if (fs::exists(File.getOutPutFilePath()))
    {
        std::cerr << "scanFlow-Error_fileIsExist \nTry to clear:" << File.getOutPutFilePath() << "\n";
        return;
    }
    QueueInterface queue;
    BinaryIO_Reader IO;
    appendMagicStatic(File.getOutPutFilePath());

    writeRoot(File); //写入当前根节点的文件数目（若选取多个文件夹，则创建一个根节点）

    IO.scanner(File, queue);

    while (!queue.fileQueue.empty())
    {

        FileDetails &details = (queue.fileQueue.front()).first;
        File.setFilePathToScan(details.getFullPath());
        IO.scanner(File, queue);

        queue.fileQueue.pop();
    }

    appendMagicStatic(File.getOutPutFilePath());
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
FileCount_Int HeaderReader::countFilesInDirectory(fs::path &filePathToScan)
{
    try
    {
        return std::distance(fs::directory_iterator(filePathToScan), fs::directory_iterator{});
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "countFilesInDirectory()-Error: " << e.what() << "\n";
        return -1;
    }
}

void HeaderReader::headerReader(std::string &path)
{

    fs::path outPutFilePath = fs::path(L"FilesList.bin"); // 直接使用 Wide 字符串
    fs::path filePathToScan;

    try
    {
        filePathToScan = _getPath(path); // 调用路径处理函数
        FilePath File(outPutFilePath, filePathToScan);
        HeaderReader reader;
        reader.scanFlow(File);
    }
    catch (const std::exception &e)
    {
        std::cerr << "headerReader()-Error: " << e.what() << std::endl;
    }
}
int main()
{
    std::string path;
    path = "D:\\1gal";

    HeaderReader begin;
    begin.headerReader(path);

    system("pause");
    return 0;
}
