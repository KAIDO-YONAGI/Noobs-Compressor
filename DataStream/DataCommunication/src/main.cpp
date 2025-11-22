#include "../include/Directory_FileProcessor.h"
#include "../include/HeaderWriter.h"
int main()
{
    std::vector<std::string> filePathToScan;
    std::string outPutFilePath, logicalRoot;

    //假装是gui获取的
    filePathToScan.push_back("D:\\1gal");
    filePathToScan.push_back("D:\\1gal\\TEST\\我挚爱的时光");
    filePathToScan.push_back("D:\\1gal\\TEST\\123意514.txt");
    outPutFilePath = "挚爱的时光.bin";
    logicalRoot = "YONAGI";

    HeaderWriter_v0 headerWriter_v0;
    headerWriter_v0.headerWriter(filePathToScan, outPutFilePath, logicalRoot);

    system("pause");
    return 0;
}