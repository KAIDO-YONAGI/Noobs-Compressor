// HeaderWriter的调试
// #include "../include/Directory_FileProcessor.h"
// #include "../include/HeaderWriter.h"
// int main()
// {
//     std::vector<std::string> filePathToScan;
//     std::string outPutFilePath, logicalRoot;

//     // 假装是gui获取的，多个文件（目录）
//     // filePathToScan.push_back("D:\\1gal\\TEST");
//     // filePathToScan.push_back("D:\\1gal\\TEST\\123意514.txt");
//     // filePathToScan.push_back("C:\\Users\\12248\\Desktop\\SFC Things\\practise");
//     filePathToScan.push_back("D:\\1gal\\1h\\Tool");

//     outPutFilePath = "挚爱的时光.bin";
//     logicalRoot = "YONAGI";

//     HeaderWriter headerWriter_v0;
//     // 默认v0,如果更新版本，可以按照以下代码指定新版函数或接口
//     //  auto customWriter = std::make_unique<HeaderWriter_v0>();
//     //  HeaderWriter writer2(std::move(customWriter));
//     //  writer2.write(files, "output_v2.bin", "/custom_root");

//     headerWriter_v0.headerWriter(filePathToScan, outPutFilePath, logicalRoot);

//     system("pause");
//     return 0;
// }

// HeaderLoader的调试
#include "../include/HeaderLoader.h"
#include "DataLoader.hpp"
#include "DataExporter.hpp"

int count =0;
void print(BinaryIO_Loader &loader)
{
    if (loader.loaderLoopIsDone() && loader.fileQueue.empty())
    {
        std::cout << "Loader is done.\n";
        return;
    }
    while (loader.fileQueue.empty() == false)
    {
        std::cout << loader.fileQueue.front().first.getFullPath() << " " << loader.fileQueue.front().second <<" "<<++count<< "\n";
        loader.fileQueue.pop();
    }
}
int main()
{
    Transfer transfer;
    std::vector<std::string> filePathToScan;
    std::string outPutFilePath, logicalRoot, inPath;

    // filePathToScan.push_back("D:\\1gal\\TEST");
    // filePathToScan.push_back("D:\\1gal\\TEST\\123意514.txt");
    // filePathToScan.push_back("C:\\Users\\12248\\Desktop\\SFC Things\\practise");
    filePathToScan.push_back("D:\\1gal\\1h\\Tool");
    outPutFilePath = "挚爱的时光.bin";
    logicalRoot = "YONAGI";
    inPath = "C:\\Users\\12248\\Desktop\\Secure Files Compressor\\DataStream\\DataCommunication\\bin\\挚爱的时光.bin";

    BinaryIO_Loader loader(inPath, filePathToScan);

    loader.headerLoader();
    print(loader);
    loader.headerLoader();
    print(loader);
    loader.headerLoader();
    print(loader);
    loader.headerLoader();
    print(loader);
    loader.headerLoader();
    print(loader);
    loader.headerLoader();
    print(loader);
    loader.headerLoader();
    print(loader);
    loader.headerLoader();
    print(loader);
    loader.headerLoader();
    print(loader);

    // while (!loader.fileQueue.empty())
    // {
    //     DataLoader dataLoader(loader.fileQueue.front().first.getFullPath());
    //     dataLoader.dataLoader();
    //     DataExporter dataExporter(transfer.transPath(inPath));
    //     dataExporter.exportDataToFile_Encryption(dataLoader.getBlock());
    //     loader.fileQueue.pop();
    //     if(loader.fileQueue.empty()&&!loader.loaderLoopIsDone()){
    //         loader.headerLoader();
    //     }

    // }
    system("pause");
}
