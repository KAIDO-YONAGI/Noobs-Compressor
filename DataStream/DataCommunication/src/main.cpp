#include "../include/HeaderReader.h"
int main()
{
    std::vector<std::string> filePathToScan;
    std::string outPutFilePath, logicalRoot;

    filePathToScan.push_back("D:\\1gal\\TEST\\Atri");

    filePathToScan.push_back("D:\\1gal\\TEST\\我挚爱的时光");

    filePathToScan.push_back("D:\\1gal\\TEST\\123意514.txt");

    outPutFilePath = "挚爱的时光.bin";

    logicalRoot = "YONAGI";
    
    HeaderReader begin;
    begin.headerReader(filePathToScan, outPutFilePath, logicalRoot);

    system("pause");
    return 0;
}
