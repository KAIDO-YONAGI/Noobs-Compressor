#include "../include/Directory_FileProcessor.h"
int main()
{
    std::vector<std::string> filePathToScan;
    std::string outPutFilePath, logicalRoot;
    MagicNumWriter writer;
    Transfer transfer;

    //假装是gui获取的
    filePathToScan.push_back("D:\\1gal");
    filePathToScan.push_back("D:\\1gal\\TEST\\我挚爱的时光");
    filePathToScan.push_back("D:\\1gal\\TEST\\123意514.txt");
    outPutFilePath = "挚爱的时光.bin";
    logicalRoot = "YONAGI";

    fs::path fullOutPath = fs::path(transfer._getPath(outPutFilePath));
    if (fs::exists(fullOutPath))
    {
        std::cerr << "HeaderWriter.cpp(main())-Error_fileIsExist\n"
                  << "Try to clear:" << fullOutPath
                  << "\n";
    }
    else{
        //写入表示文件起始的魔数
        writer.appendMagicStatic(fullOutPath);

        //处理从gui获取的路径
        Directory_FileProcessor begin;
        begin.directory_fileProcessor(filePathToScan, outPutFilePath, logicalRoot);  
    }


    system("pause");
    return 0;
}