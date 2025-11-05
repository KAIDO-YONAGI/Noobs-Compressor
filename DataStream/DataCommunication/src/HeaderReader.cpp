#include "../include/HeaderReader.h"

// void Locator::relativeLocator(std::ofstream &File, int offset)
// {
//     File.seekp(File.tellp() + offset, File.beg)；
// }
// void Locator::relativeLocator(std::ifstream &File, int offset)
// {
//     File.seekg(File.tellg() + offset, File.beg)；
// }
void readerForCompression(){
    std::vector<std::string> tempDirectoryPath;
}
void readerForDecompression(){
    std::vector<std::string> tempDirectoryPath;
}
void scanFlow(FilePath &File)
{
    if (fileIsExist(File.getOutPutFilePath()))
    {
        std::cerr << ("scanFlow-Error_fileIsExist \nTry to clear:" + std::string(File.getOutPutFilePath()))<< "\n";;
        return ;
    }
    BinaryIO headerReader(File);
    appendMagicStatic(File.getOutPutFilePath());
    headerReader.scanner();
    appendMagicStatic(File.getOutPutFilePath());
}

bool fileIsExist(const char *outPutFilePath)
{
    FILE *file = fopen(outPutFilePath, "r");
    if (file)
    {
        fclose(file);
        return true;
    }
    else
        return false;
}

bool isFile(const char *outPutFilePath)
{
    struct stat statbuf;
    if (stat(outPutFilePath, &statbuf) != 0)
    {
        std::cerr << (("isFile()-Error_stat():" + std::string(outPutFilePath)).c_str())<< "\n";;
        return false;
    }
    return S_ISREG(statbuf.st_mode);
}

void appendMagicStatic(const char *outputFilePath)
{
    // 自动以追加模式打开文件
    std::ofstream outFile(outputFilePath, std::ios::binary | std::ios::app);

    if (!outFile)
    {
        std::cerr << "appendMagicStatic-Error_failToOpenFile: " << outputFilePath << "\n";
        return;
    }

    // 写入32位幻数0xDEADBEEF
    const uint32_t magic = 0xDEADBEEF;
    outFile.write((const char *)&magic, sizeof(magic));

    if (!outFile)
    {
        std::cerr << "Error_Can't write magic num in file"<< "\n";
    }
    outFile.close();
}
uint64_t BinaryIO::getFileSize(const char *filePathToScan)
{
    POSIX_STAT stat_buf;
    uint64_t outSize;
    if (POSIX_STAT_FUNC(filePathToScan, &stat_buf) != 0)
    {
        std::cerr << ("getFileSize()-Error_stat()")<<"\n"; // 打印错误
    }
    outSize = stat_buf.st_size;
    return outSize;
}
void BinaryIO::scanner()
{
    
    FileQueue queue;
    bool goingScan=true;
    while (1)
    {
        std::ofstream outfile(File.getOutPutFilePath(), std::ios::binary | std::ios::app);
  
        POSIX_DIR *dir = opendir(File.getFilePathToScan());

        struct POSIX_DIRENT *entry;
        if (!dir)
        {
            std::cerr << ("scanner-Error_failToOpenFile:"+std::string(File.getFilePathToScan()))<<"\n";
            return;
        }

        goingScan=false;

        while ((entry = readdir(dir)) != NULL)
        {
            goingScan=true;
            const std::string name = entry->d_name;
            if (name == "." || name == "..")
                continue;
            const std::string fullPath = std::string(File.getFilePathToScan()) + "\\" + name; //拼接路径
            // File.setFilePathToScan(fullPath.c_str());
            uint8_t sizeOfName = name.size();
            bool is_File = isFile(fullPath.c_str());
            uint64_t fileSize = getFileSize(fullPath.c_str()); //获取文件大小

            FileDetails details(name, sizeOfName, fileSize, is_File, fullPath); //构造文件信息
            writeBinaryStandard(outfile, details, queue);                       //写入存储协议、目录树
        }
        POSIX_CLOSEDIR(dir);
        
        if(!goingScan&&queue.fileQueue.empty())break;
        
        int count=0;

        // while (!queue.fileQueue.empty()){

        //     FileDetails details = queue.fileQueue.front().first;

        //     File.setFilePathToScan(details.getFullPath().c_str());

        //     int count = queue.fileQueue.front().second;

        //     queue.fileQueue.pop();

        // }

        outfile.close();
    }
    
    return;
}
void BinaryIO::writeBinaryStandard(std::ofstream &outfile, FileDetails &details, FileQueue &queue)
{
    if (details.getIsFile())
    {
        writeFileStandard(outfile, details);
    }
    else if (!details.getIsFile())
    {
        int countOfThisHeader = countFilesInDirectory(details.getFullPath().c_str());

        if(countOfThisHeader>=0){
            queue.fileQueue.push({details, countOfThisHeader});
            writeHeaderStandard(outfile, details, countOfThisHeader);
        }

    }
}
void BinaryIO::writeFileStandard(std::ofstream &outfile, FileDetails &details)
{
    uint8_t SizeOfName = details.getSizeOfName();
    write_binary_le(outfile, SizeOfName);                 //文件名偏移量
    outfile.write(details.getName().c_str(), SizeOfName); //文件名
    outfile.write("1", 1);                                //文件标识
    write_binary_le(outfile, details.getFileSize());      //压缩前大小
    write_binary_le(outfile, uint64_t(0));                //预留64位供压缩完成后使用
}
void BinaryIO::writeHeaderStandard(std::ofstream &outfile, FileDetails &details, uint32_t count)
{
    
    uint8_t SizeOfName = details.getSizeOfName();
    write_binary_le(outfile, SizeOfName);                 //文件名偏移量
    outfile.write(details.getName().c_str(), SizeOfName); //文件名
    outfile.write("0", 1);
    write_binary_le(outfile, count); //写入目录下文件数目
}
int countFilesInDirectory(const char *filePathToScan)
{
    POSIX_DIR *dir = opendir(filePathToScan);
    if (dir == NULL)
    {
        std::cerr<<("writeHeaderStandard()-Error_failongsToOpenFile:"+std::string(filePathToScan))<<"\n";
        return -1;
    }

    int count = 0;
    struct POSIX_DIRENT *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        // 跳过当前目录和上级目录
        if (entry->d_name=="." || entry->d_name=="..")
        {
            continue;
        }
        count++;
    }

    POSIX_CLOSEDIR(dir);
    return count;
}
int main()
{
    std::locale::global(std::locale(""));
    const char *outPutFilePath = "FilesList.bin";
    const char *filePathToScan = "D:\\1gal";

    FilePath File(outPutFilePath, filePathToScan);

    scanFlow(File);

    system("pause");
    return 0;
}
