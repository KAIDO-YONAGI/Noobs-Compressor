// HeaderWriter的调试
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

    HeaderWriter headerWriter_v0;
    //默认v0,如果更新版本，可以按照以下代码指定新版函数或接口
    // auto customWriter = std::make_unique<HeaderWriter_v0>();
    // HeaderWriter writer2(std::move(customWriter));
    // writer2.write(files, "output_v2.bin", "/custom_root");
    
    headerWriter_v0.headerWriter(filePathToScan, outPutFilePath, logicalRoot);

    system("pause");
    return 0;
}

//HeaderLoader的调试
// #include"../include/HeaderLoader.h"
// int main(){
    
// }