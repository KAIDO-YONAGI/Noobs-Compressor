// 完整Loop
#include "../EncryptionModules/Aes/include/Aes.h"
#include "../CompressorFileSystem/DataCommunication/include/HeaderWriter.h"
#include "../CompressorFileSystem/DataCommunication/include/HeaderLoader.h"

int main()
{
    std::vector<std::string> filePathToScan;
    std::string logicalRoot, compressionFilePath;
    const char *key;
    // filePathToScan.push_back("D:\\1gal\\TEST");
    // filePathToScan.push_back("D:\\1gal\\TEST\\123意514.txt");
    // filePathToScan.push_back("D:\\1gal\\TEST\\我");
    // filePathToScan.push_back("D:\\1gal\\Goods\\拔作岛\\2\\root.pfs.020");
    filePathToScan.push_back("D:\\1gal\\1h\\Tool\\node_modules");
    // filePathToScan.push_back("D:\\1gal\\1h\\Tool\\credits.html");

    logicalRoot = "YONAGI";
    compressionFilePath = "C:\\Users\\12248\\Desktop\\Secure Files Compressor\\Y_Manager\\bin\\挚爱的时光.bin";
    key = "LOVEYONAGI";

    // 默认v0,如果更新版本，可以按照以下代码指定新版函数或接口
    //  auto customWriter = std::make_unique<HeaderWriter_v0>();
    //  HeaderWriter writer2(std::move(customWriter));
    //  writer2.write(files, "output_v2.bin", "/custom_root");

    Aes aes(key);

    HeaderWriter headerWriter_v0;
    headerWriter_v0.headerWriter(filePathToScan, compressionFilePath, logicalRoot);

    HeaderLoader_Compression headerLoader_Compression(compressionFilePath);
    headerLoader_Compression.headerLoader( filePathToScan, aes);

    system("pause");
}
// int main()
// {
//     std::string deCompressionFilePath = "C:\\Users\\12248\\Desktop\\Secure Files Compressor\\Y_Manager\\bin\\挚爱的时光.bin";
//     const char *key="LOVEYONAGI";

//     Aes aes(key);

//     HeaderLoader_Decompression headerLoader_Decompression(deCompressionFilePath);
//     headerLoader_Decompression.headerLoader(aes);

//     system("pause");
// }