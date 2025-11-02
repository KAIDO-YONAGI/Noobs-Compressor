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

bool fileIsExist(const std::string &outPutFilePath){
    FILE* file=fopen(outPutFilePath.c_str(),"r");
    if(file){
        fclose(file);
        return true;
    }
    else return false;
}

bool isFile(const char* path){
    struct stat statbuf;
    
    if (stat(path, &statbuf) != 0) {
        std::cerr <<(("error_stat: " + std::string(path)).c_str());
        return false;
    }
    return S_ISREG(statbuf.st_mode);
} 

void appendMagicStatic(const std::string& outputFilePath) {
    // 以二进制追加模式打开文件
    std::ofstream outFile(outputFilePath, std::ios::binary | std::ios::app);
    
    if (!outFile) {
        std::cerr << "Can't open output file: " << outputFilePath <<"\n";
        return;
    }

    // 写入32位魔数（0xDEADBEEF）
    const uint32_t magic = 0xDEADBEEF;
    outFile.write(reinterpret_cast<const char*>(&magic), sizeof(magic));

    if (!outFile) {
        std::cerr << "Error:Can't write magic num in file" << "\n";
    }
    outFile.close();
}

void outPutAllPaths(const std::string &outPutFilePath,const std::string &filePathToScan)
{
    POSIX_DIR *dir=opendir(filePathToScan.c_str());
    struct POSIX_DIRENT *entry;
    
    if (!dir) {
        std::cerr <<("error_failToOpenFile");
        return;
    }
    std::ofstream file(outPutFilePath, std::ios::binary | std::ios::app);
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;
        std::string fullPath=filePathToScan+"\\"+name;//绝对路径
        uint64_t sizeOfName=name.size();
        write_binary_le(file,sizeOfName);
        file.write(name.c_str(),sizeOfName);
        file.write(isFile(fullPath.c_str())?"1":"0",1);
    }
    file.close();
    POSIX_CLOSEDIR(dir);
}

void scanFlow(const std::string &outPutFilePath,const std::string &filePathToScan){
    if(fileIsExist(outPutFilePath)){
        std::cerr <<("Error:fileIsExist \nTry to clear:"+outPutFilePath);
    }
    appendMagicStatic(outPutFilePath);
    outPutAllPaths(outPutFilePath, filePathToScan);
    appendMagicStatic(outPutFilePath);

}

int main()
{
    const std::string outPutFilePath="FilesList.bin";
    const std::string filePathToScan="D:\\1gal";
    
    scanFlow(outPutFilePath,filePathToScan);

    system("pause");
    return 0;
}
