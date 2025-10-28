#include "../include/HeaderReader.h"

void readerForCompression(){
    vector<string> tempDirectoryPath;

}

void readerForDecompression(){
    vector<string> tempDirectoryPath;
}

void listFiles(const fs::path &basePath, const fs::path &relativePath, std::vector<std::string> &files)
{
    fs::path fullPath = basePath / relativePath;  // 使用 / 运算符拼接路径

    // 遍历目录（recursive_iterator 可以递归遍历所有子目录）
    for (const auto &entry : fs::directory_iterator(fullPath))
    {
        fs::path filePath = entry.path();  // 当前文件/目录的完整路径
        fs::path relative = relativePath.empty() ? entry.path().filename() : relativePath / entry.path().filename();
        
        // 不需要手动检查 "." 或 ".."，C++17已经自动过滤
        files.push_back(relative.string());  // 存入相对路径

        if (entry.is_directory())  // 如果是子目录，递归处理
        {
            listFiles(basePath, relative, files);
        }
    }
}
void appendMagicStatic(const string& outputFilePath) {
    // 以二进制追加模式打开文件
    ofstream outFile(outputFilePath, ios::binary | ios::app);
    
    if (!outFile) {
        std::cerr << "Can't open output file: " << outputFilePath << endl;
        return;
    }

    // 写入32位魔数（0xDEADBEEF）
    const uint32_t magic = 0xDEADBEEF;
    outFile.write(reinterpret_cast<const char*>(&magic), sizeof(magic));

    if (!outFile) {
        std::cerr << "Error:Can't write magic num in file" << endl;
    }
    return;
    outFile.close();
}
void outPutAllPaths(string &outPutFilePath, string &filePathToScan)
{
    vector<string> files;
    listFiles(filePathToScan, "", files);
    ofstream outFile(outPutFilePath,ios::binary | ios::app);

    for (const auto &file : files)
    {
        outFile << file<<"\n";
        // cout << file << endl;
    }
    outFile.close(); //需要注意关闭时机
}
bool fileIsExist(string &outPutFilePath){
    FILE* file=fopen(outPutFilePath.c_str(),"r");
    if(file)return true;
    else return false;
}
void scanFlow(string &outPutFilePath, string &filePathToScan){
    if(fileIsExist(outPutFilePath)){
        throw runtime_error("Error:fileIsExist \nTry to clear:"+outPutFilePath);
    }
    appendMagicStatic(outPutFilePath);
    outPutAllPaths(outPutFilePath, filePathToScan);
    appendMagicStatic(outPutFilePath);
}
int main()
{
    string filePathToScan = "";
    string outPutFilePath = "";
    filePathToScan="D:\\1gal";//test
    outPutFilePath="FilesList.bin";
    try{
        scanFlow(outPutFilePath,filePathToScan);
        cout << "Resluts have been put to:" << outPutFilePath << endl;
    }catch(runtime_error& e){
        cerr<<e.what()<<"\n";
    }
    

    // cout << "Enter filePathToScan:";
    // cin >> filePathToScan;

    
    // cout << "Enter outPutFilePath:";
    // cin>>outPutFilePath;
    
    system("pause");
    return 0;
}