#include "../include/HeaderReader.h"


void readerForCompression(){
    std::vector<std::string> tempDirectoryPath;

}

void readerForDecompression(){
    std::vector<std::string> tempDirectoryPath;
}
void Locator:: relativeLocator(std::ofstream& File,int offset){
File.seekp(File.tellp()+offset,File.beg);
}
void Locator:: relativeLocator(std::ifstream& File,int offset){
File.seekg(File.tellg()+offset,File.beg);
}
void listFiles(const std::string &basePath, const std::string &relativePath, std::vector<std::string> &files)
{
    std::string fullPath = basePath + "/" + relativePath;
    POSIX_DIR *dir = POSIX_OPENDIR(fullPath.c_str());
    POSIX_DIRENT *entry;

    while ((entry = POSIX_READDIR(dir)) != nullptr)
    {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;

        std::string newRelativePath = relativePath.empty() ? name : relativePath + "/" + name;
        files.push_back(newRelativePath);

        POSIX_STAT statbuf;
        POSIX_STAT_FUNC((fullPath + "/" + name).c_str(), &statbuf);
        if (POSIX_S_ISDIR(statbuf.st_mode))
        {
            listFiles(basePath, newRelativePath, files);
        }
    }
    POSIX_CLOSEDIR(dir);
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
    return;
    outFile.close();
}
void outPutAllPaths(std::string &outPutFilePath, std::string &filePathToScan)
{
    std::vector<std::string> files;
    listFiles(filePathToScan, "", files);
    std::ofstream outFile(outPutFilePath,std::ios::binary | std::ios::app);

    for (const auto &file : files)
    {
        outFile << file<<"\n";
        // cout << file << endl;
    }
    outFile.close(); //需要注意关闭时机
}
bool fileIsExist(std::string &outPutFilePath){
    FILE* file=fopen(outPutFilePath.c_str(),"r");
    if(file)return true;
    else return false;
}
void scanFlow(std::string &outPutFilePath, std::string &filePathToScan){
    if(fileIsExist(outPutFilePath)){
        throw std::runtime_error("Error:fileIsExist \nTry to clear:"+outPutFilePath);
    }
    appendMagicStatic(outPutFilePath);
    outPutAllPaths(outPutFilePath, filePathToScan);
    appendMagicStatic(outPutFilePath);
}
int main()
{
    // string filePathToScan = "";
    // string outPutFilePath = "";
    // filePathToScan="D:\\1gal";//test
    // outPutFilePath="FilesList.bin";
    // try{
    //     scanFlow(outPutFilePath,filePathToScan);
    //     cout << "Resluts have been put to:" << outPutFilePath << endl;
    // }catch(runtime_error& e){
    //     cerr<<e.what()<<"\n";
    // }
    

    // cout << "Enter filePathToScan:";
    // cin >> filePathToScan;

    
    // cout << "Enter outPutFilePath:";
    // cin>>outPutFilePath;

//二进制数值读写

    // std::ofstream file("data.bin", std::ios::binary);
    //     if (!file) {
    //         std::cerr << "Failed to open file for writing!" << std::endl;
    //         return 1;
    //     }
    //     uint64_t num1 = 0x12345678DEADBEEF;

    //     write_binary_le(file, num1);

    //     std::cout << "Successfully wrote numbers to file." << std::endl;


    // std::ifstream file("data.bin", std::ios::binary);
    // if (!file) {
    //     std::cerr << "Failed to open file for reading!" << std::endl;
    //     return 1;
    // }

    // uint64_t num1 = read_binary_le<uint64_t>(file);

    // std::cout << "Read values:" << std::endl;
    // std::cout << "num1 : 0x" << std::hex << num1 << std::endl;


    getchar(); 
    return 0;
}