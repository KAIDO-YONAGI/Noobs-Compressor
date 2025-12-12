// HeaderWriter的调试
// #include "../CompressorFileSystem/DataCommunication/include/HeaderWriter.h"
// int main()
// {
//     std::vector<std::string> filePathToScan;
//     std::string outPutFilePath, logicalRoot;

//     // 假装是gui获取的，多个文件（目录）
//     // filePathToScan.push_back("D:\\1gal\\TEST");
//     // filePathToScan.push_back("D:\\1gal\\TEST\\123意514.txt");
//     // filePathToScan.push_back("C:\\Users\\12248\\Desktop\\SFC Things\\practise");
//     filePathToScan.push_back("D:\\1gal\\1h\\Tool\\node_modules");

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

// #include "../CompressorFileSystem/DataCommunication/include/HeaderLoader.h"
// #include "../CompressorFileSystem/DataCommunication/include/DataLoader.h"
// #include "../CompressorFileSystem/DataCommunication/include/DataExporter.h"

// int main()
// {
//     Transfer transfer;
//     std::vector<std::string> filePathToScan;
//     std::string logicalRoot, compressionFilePath;

//     // filePathToScan.push_back("D:\\1gal\\TEST");
//     // filePathToScan.push_back("D:\\1gal\\TEST\\123意514.txt");
//     // filePathToScan.push_back("C:\\Users\\12248\\Desktop\\SFC Things\\practise");
//     filePathToScan.push_back("D:\\1gal\\1h\\Tool\\node_modules");
//     logicalRoot = "YONAGI";
//     compressionFilePath = "C:\\Users\\12248\\Desktop\\Secure Files Compressor\\Y_Manager\\bin\\挚爱的时光.bin";

//     // 初始化加载器
//     BinaryIO_Loader_Compression headerLoader(compressionFilePath, filePathToScan);
//     headerLoader.headerLoader(); // 执行第一次操作，把根目录载入
//     Locator locator;

//     fs::path loadPath = headerLoader.fileQueue.front().first.getFullPath();
//     DataLoader dataLoader(loadPath);
//     DataExporter dataExporter(transfer.transPath(compressionFilePath));
//     int count = 0;
//     while (!headerLoader.fileQueue.empty())
//     {

//         dataLoader.dataLoader();

//         if (!dataLoader.isDone()) // 避免读到空数据块
//         {
//             dataExporter.exportDataToFile_Encryption(dataLoader.getBlock()); // 读取的数据传输给exporter
//         }
//         else if (dataLoader.isDone() && !headerLoader.fileQueue.empty())
//         {
//             FileNameSize_uint offsetToFill = headerLoader.fileQueue.front().second;
//             dataExporter.thisFileIsDone(offsetToFill);//可在此前插入一个编码表写入再调用done
//             std::cout << "Loaded file: " << headerLoader.fileQueue.front().first.getFullPath() << " " << ++count << "\n";
//             headerLoader.fileQueue.pop();
//             if (!headerLoader.fileQueue.empty())// 更新下一个文件路径，生成编码表时可以调用reset（原目录）读两轮
//                 dataLoader.reset(headerLoader.fileQueue.front().first.getFullPath());
//         }

//         if (headerLoader.fileQueue.empty() && !headerLoader.allLoopIsDone())// 队列空但整体未完成，请求下一轮读取对队列进行填充
//         {
//             headerLoader.restartLoader();
//             headerLoader.headerLoader();
//         }
//     }
//     system("pause");
// }
// 完整Loop
#include "../CompressorFileSystem/DataCommunication/include/HeaderWriter.h"
#include "../CompressorFileSystem/DataCommunication/include/HeaderLoader.h"

int main()
{
    std::vector<std::string> filePathToScan;
    std::string logicalRoot, compressionFilePath;

    // filePathToScan.push_back("D:\\1gal\\TEST");
    // filePathToScan.push_back("C:\\Users\\12248\\Desktop\\SFC Things\\practise");
    // filePathToScan.push_back("D:\\1gal\\1h\\Tool\\node_modules");
    // filePathToScan.push_back("D:\\1gal\\TEST\\123意514.txt");
    filePathToScan.push_back("D:\\1gal\\TEST\\我");
    logicalRoot = "YONAGI";
    compressionFilePath = "C:\\Users\\12248\\Desktop\\Secure Files Compressor\\Y_Manager\\bin\\挚爱的时光.bin";

    
    // 默认v0,如果更新版本，可以按照以下代码指定新版函数或接口
    //  auto customWriter = std::make_unique<HeaderWriter_v0>();
    //  HeaderWriter writer2(std::move(customWriter));
    //  writer2.write(files, "output_v2.bin", "/custom_root");
    HeaderWriter headerWriter_v0;
    headerWriter_v0.headerWriter(filePathToScan, compressionFilePath, logicalRoot);

    HeaderLoader_Compression headerLoader_Compression;
    headerLoader_Compression.headerLoader(compressionFilePath, filePathToScan);

    system("pause");
}
