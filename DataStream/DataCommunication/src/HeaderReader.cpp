#include "../include/HeaderReader.h"

// void Locator:: relativeLocator(std::ofstream& File,int offset){
//     File.seekp(File.tellp()+offset,File.beg);
// }
// void Locator:: relativeLocator(std::ifstream& File,int offset){
//     File.seekg(File.tellg()+offset,File.beg);
// }
// void readerForCompression(){
//     std::vector<std::string> tempDirectoryPath;
// }
// void readerForDecompression(){
//     std::vector<std::string> tempDirectoryPath;
// }

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
        std::cerr << (("error_stat(): " + std::string(outPutFilePath)).c_str());
        return false;
    }
    return S_ISREG(statbuf.st_mode);
}

void appendMagicStatic(const char *outputFilePath)
{
    // 以二进制追加模式打开文件
    std::ofstream outFile(outputFilePath, std::ios::binary | std::ios::app);

    if (!outFile)
    {
        std::cerr << "Can't open output file: " << outputFilePath << "\n";
        return;
    }

    // 写入32位魔数（0xDEADBEEF）
    const uint32_t magic = 0xDEADBEEF;
    outFile.write((const char *)&magic, sizeof(magic));

    if (!outFile)
    {
        std::cerr << "Error:Can't write magic num in file"
                  << "\n";
    }
    outFile.close();
}
uint64_t BinaryIO::getFileSize(const char *filePathToScan)
{
    struct stat stat_buf;
    uint64_t outSize;
    if (stat(filePathToScan, &stat_buf) != 0)
    {
        std::cerr << ("error_stat()"); // 打印错误
    }
    outSize = stat_buf.st_size;
    return outSize;
}
void BinaryIO::scanner()
{
    POSIX_DIR *dir = opendir(File.getFilePathToScan());
    struct POSIX_DIRENT *entry;
    if (!dir)
    {
        std::cerr << ("error_failToOpenFile");
        return;
    }
    std::ofstream outfile(File.getOutPutFilePath(), std::ios::binary | std::ios::app);
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        std::string fullPath = std::string(File.getFilePathToScan()) + "\\" + name; //绝对路径
        uint8_t sizeOfName = name.size();
        bool is_File = isFile(fullPath.c_str());
        uint64_t fileSize = getFileSize(fullPath.c_str()); //获取文件大小

        if (name == "." || name == "..")
            continue;

        FileDetails details(name, sizeOfName, fileSize, is_File, fullPath); //保存文件信息
        WriteBinaryStandard(outfile, details);                              //写入存储协议
    }
    outfile.close();
    POSIX_CLOSEDIR(dir);
    return;
}
void BinaryIO::WriteBinaryStandard(std::ofstream &outfile, FileDetails &details)
{
    if (details.getIsFile())
    {
        uint8_t SizeOfName = details.getSizeOfName();

        write_binary_le(outfile, SizeOfName);                 //文件名偏移量
        outfile.write(details.getName().c_str(), SizeOfName); //文件名
        outfile.write("1", 1);                                //文件标记
        write_binary_le(outfile, details.getFileSize());      //压缩前大小
        write_binary_le(outfile, uint64_t(0));                //预留64位，压缩完成后使用
    }
    else if (!details.getIsFile())
    {
        outfile.write("0", 1);
    }
}
void scanFlow(FilePath &File)
{
    if (fileIsExist(File.getOutPutFilePath()))
    {
        std::cerr << ("Error:fileIsExist \nTry to clear:" + std::string(File.getOutPutFilePath()));
    }
    BinaryIO headerReader(File);
    appendMagicStatic(File.getOutPutFilePath());
    headerReader.scanner();
    appendMagicStatic(File.getOutPutFilePath());
}

int main()
{
    const char *outPutFilePath = "FilesList.bin";
    const char *filePathToScan = "D:\\1gal";
    
    FilePath File(outPutFilePath, filePathToScan);

    scanFlow(File);

    system("pause");
    return 0;
}
