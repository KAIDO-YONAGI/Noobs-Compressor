#include "../include/HeaderReader.h"

// void Locator:: relativeLocator(std::ofstream& File,int offset){
//     File.seekp(File.tellp()+offset,File.beg);
// }
// void Locator:: relativeLocator(std::ifstream& File,int offset){
//     File.seekg(File.tellg()+offset,File.beg);
// }
void readerForCompression(){
    std::vector<std::string> tempDirectoryPath;
}
void readerForDecompression(){
    std::vector<std::string> tempDirectoryPath;
}

bool fileIsExist(const char* outPutFilePath){
    FILE* file=fopen(outPutFilePath,"r");
    if(file){
        fclose(file);
        return true;
    }
    else return false;
}

bool isFile(const char* path){
    struct stat statbuf;
    
    if (stat(path, &statbuf) != 0) {
        std::cerr <<(("error_stat(): " + std::string(path)).c_str());
        return false;
    }
    return S_ISREG(statbuf.st_mode);
} 

void appendMagicStatic(const char* outputFilePath) {
    // 以二进制追加模式打开文件
    std::ofstream outFile(outputFilePath, std::ios::binary | std::ios::app);
    
    if (!outFile) {
        std::cerr << "Can't open output file: " << outputFilePath <<"\n";
        return;
    }

    // 写入32位魔数（0xDEADBEEF）
    const uint32_t magic = 0xDEADBEEF;
    outFile.write((const char*)&magic, sizeof(magic));

    if (!outFile) {
        std::cerr << "Error:Can't write magic num in file" << "\n";
    }
    outFile.close();
}
uint64_t getFileSize(const char* filepath) {
    struct stat stat_buf;
    uint64_t outSize;
    if (stat(filepath, &stat_buf) != 0) {
        std::cerr <<("error_stat()"); // 打印错误
    }
    outSize = stat_buf.st_size;
    return outSize;
}
void outPutAllPaths(const char* outPutFilePath, const char* filePathToScan)
{
    POSIX_DIR *dir=opendir(filePathToScan);
    struct POSIX_DIRENT *entry;
    
    if (!dir) {
        std::cerr <<("error_failToOpenFile");
        return;
    }
    std::ofstream file(outPutFilePath, std::ios::binary | std::ios::app);
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        std::string fullPath = std::string(filePathToScan) + "\\" + name; //绝对路径
        uint8_t sizeOfName=name.size();
        bool is_File=isFile(fullPath.c_str());

        if (name == "." || name == "..")
            continue;

        write_binary_le(file,sizeOfName);//文件名偏移量
        file.write(name.c_str(),sizeOfName);//文件名

        file.write(is_File?"1":"0",1);
        
        if(is_File){
            uint64_t fileSize=getFileSize(fullPath.c_str());
            write_binary_le(file,fileSize);
            write_binary_le(file,uint64_t(0));
        }
        else if(!is_File){
        }
    }
    file.close();
    POSIX_CLOSEDIR(dir);
}

void scanFlow(const char* outPutFilePath, const char* filePathToScan){
    if(fileIsExist(outPutFilePath)){
        std::cerr <<("Error:fileIsExist \nTry to clear:" + std::string(outPutFilePath));
    }
    appendMagicStatic(outPutFilePath);
    outPutAllPaths(outPutFilePath, filePathToScan);
    appendMagicStatic(outPutFilePath);
}

int main()
{
    const char* outPutFilePath="FilesList.bin";
    const char* filePathToScan="D:\\1gal";
    
    scanFlow(outPutFilePath, filePathToScan);

    system("pause");
    return 0;
}
