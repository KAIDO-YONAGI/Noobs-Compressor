// HeaderReader.cpp
#include "../include/HeaderReader.h"
// void Locator:: relativeLocator(std::ofstream& File,int offset){
//     File.seekp(File.tellp()+offset,File.beg);
// }
// void Locator:: relativeLocator(std::ifstream& File,int offset){
//     File.seekg(File.tellg()+offset,File.beg);
// }
// void readerForCompression()
// {
//     std::vector<fs::path> tempDirectoryPath;
// }

// void readerForDecompression()
// {
//     std::vector<fs::path> tempDirectoryPath;
// }

bool fileIsExist(const fs::path &outPutFilePath)
{
    return fs::exists(outPutFilePath);
}

bool isFile(const fs::path &outPutFilePath)
{
    return fs::is_regular_file(outPutFilePath);
}

void appendMagicStatic(const fs::path &outputFilePath)
{
    std::ofstream outFile(outputFilePath, std::ios::binary | std::ios::app);
    if (!outFile)
    {
        std::cerr << "appendMagicStatic-Error_failToOpenFile: " << outputFilePath << "\n";
        return;
    }

    const uint32_t magic = 0xDEADBEEF;

    write_binary_le(outFile,magic);
}

uint64_t BinaryIO::getFileSize(const fs::path &filePathToScan)
{
    try
    {
        return fs::file_size(filePathToScan);
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "getFileSize()-Error: " << e.what() << "\n";
        return 0;
    }
}

void BinaryIO::scanner(FilePath& File,FileQueue& queue)
{
    bool goingScan = true;

    std::ofstream outfile(File.getOutPutFilePath(), std::ios::binary | std::ios::app);
    if (!outfile)
    {
        std::cerr << "scanner()-Error_failToOpenFile:" << File.getFilePathToScan() << "\n";
        return;
    }

    goingScan = false;

    try
    {
        for (const auto &entry : fs::directory_iterator(File.getFilePathToScan()))
        {
            goingScan = true;
            const std::string name = entry.path().filename().string();
            if (name == "." || name == "..")
                continue;

            auto fullPath = entry.path();
            uint8_t sizeOfName = name.size();
            bool is_File = entry.is_regular_file();
            uint64_t fileSize = is_File ? entry.file_size() : 0;

            FileDetails details(name, sizeOfName, fileSize, is_File, fullPath);//创建details
            writeBinaryStandard(outfile, details, queue);
        }

    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "scanner()-Error: " << e.what() << "\n";
    }


    outfile.close();

}

void BinaryIO::writeBinaryStandard(std::ofstream &outfile, FileDetails &details, FileQueue &queue)
{
    if (details.getIsFile())
    {
        writeFileStandard(outfile, details);
    }
    else
    {
        uint32_t countOfThisHeader = countFilesInDirectory(details.getFullPath());
        if (countOfThisHeader >= 0)
        {
            queue.fileQueue.push({details, countOfThisHeader});
            writeHeaderStandard(outfile, details, countOfThisHeader);
        }
    }
}

void BinaryIO::writeFileStandard(std::ofstream &outfile, FileDetails &details)
{
    uint8_t SizeOfName = details.getSizeOfName();
    outfile.write("1", 1);//先写文件标
    write_binary_le(outfile, SizeOfName);//写入文件名偏移量
    outfile.write(details.getName().c_str(), SizeOfName);//写入文件名
    write_binary_le(outfile, details.getFileSize());//写入文件大小
    write_binary_le(outfile, uint64_t(0));//预留大小
}

void BinaryIO::writeHeaderStandard(std::ofstream &outfile, FileDetails &details, uint32_t count)
{
    uint8_t SizeOfName = details.getSizeOfName();
    outfile.write("0", 1);
    write_binary_le(outfile, SizeOfName);
    outfile.write(details.getName().c_str(), SizeOfName);
    write_binary_le(outfile, count);//写入文件数目
}

uint32_t countFilesInDirectory(const fs::path &filePathToScan)
{
    try
    {
        return std::distance(fs::directory_iterator(filePathToScan), fs::directory_iterator{});
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "countFilesInDirectory()-Error: " << e.what() << "\n";
        return -1;
    }
}

void scanFlow(FilePath &File)
{
    if (fileIsExist(File.getOutPutFilePath()))
    {
        std::cerr << "scanFlow-Error_fileIsExist \nTry to clear:" << File.getOutPutFilePath() << "\n";
        return;
    }
    FileQueue queue;
    BinaryIO IO;
    appendMagicStatic(File.getOutPutFilePath());
    
    do{

        if(!queue.fileQueue.empty()){
            FileDetails& details = queue.fileQueue.front().first;

            File.setFilePathToScan(details.getFullPath());  
        }
        
        IO.scanner(File,queue);

        queue.fileQueue.pop();
    }while (!queue.fileQueue.empty());

    appendMagicStatic(File.getOutPutFilePath());
}

int main()
{
    fs::path outPutFilePath = "FilesList.bin";
    fs::path filePathToScan = "D:/1gal";

    FilePath File(outPutFilePath, filePathToScan);
    scanFlow(File);

    system("pause");
    return 0;
}
