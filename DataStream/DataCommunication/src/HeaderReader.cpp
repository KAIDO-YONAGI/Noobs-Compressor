#include "../include/HeaderReader.h"

void readerForCompression(){
    vector<string> tempDirectoryPath;

}

void readerForDecompression(){
    vector<string> tempDirectoryPath;
}
void Locator:: relativeLocator(std::ofstream& File,int offset){
File.seekp(File.tellp()+offset,File.beg);
}
void Locator:: relativeLocator(std::ifstream& File,int offset){
File.seekg(File.tellg()+offset,File.beg);
}
void listFiles(const string &basePath, const string &relativePath, vector<string> &files)
{
    string fullPath = basePath + "/" + relativePath;
    POSIX_DIR *dir = POSIX_OPENDIR(fullPath.c_str());
    POSIX_DIRENT *entry;

    while ((entry = POSIX_READDIR(dir)) != nullptr)
    {
        string name = entry->d_name;
        if (name == "." || name == "..")
            continue;

        string newRelativePath = relativePath.empty() ? name : relativePath + "/" + name;
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