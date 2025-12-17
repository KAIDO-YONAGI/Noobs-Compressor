// 完整Loop
#include "../EncryptionModules/Aes/include/My_Aes.h"
#include "../CompressorFileSystem/DataCommunication/include/HeaderWriter.h"
#include "MainLoop.h"

int main()
{
    std::vector<std::string> filePathToScan;
    std::string logicalRoot, compressionFilePath;
    const char *key;
    // filePathToScan.push_back("D:\\1gal\\TEST");
    filePathToScan.push_back("D:\\1gal\\1h\\Tool\\node_modules");
    // filePathToScan.push_back("D:\\1gal\\1h\\Tool\\node_modules\\adm-zip\\methods\\deflater.js");
    // filePathToScan.push_back("D:\\1gal\\1h\\Tool\\node_modules\\pe-library\\dist\\_esm\\type\\index.d.ts");

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

    CompressionLoop compressor(compressionFilePath);
    compressor.compressionLoop(filePathToScan, aes);

    system("pause");
}

// int main()
// {
//     std::string deCompressionFilePath = "C:\\Users\\12248\\Desktop\\Secure Files Compressor\\Y_Manager\\bin\\挚爱的时光.bin";
//     const char *key = "LOVEYONAGI";

//     Aes aes(key);

//     DecompressionLoop decompressor(deCompressionFilePath);
//     decompressor.decompressionLoop(aes);

//     system("pause");
// }