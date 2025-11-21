#include "../include/Directory_FileProcessor.h"
int main()
{
    std::vector<std::string> filePathToScan;
    std::string outPutFilePath, logicalRoot;
    MagicNumWriter numWriter;
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
    else
    {
        std::ofstream outFile(fullOutPath, std::ios::binary | std::ios::out);

        //写入表示文件起始的4字节魔数
        numWriter.appendMagicStatic(fullOutPath);

        //文件头
        try
        {
            Locator locator;
            
            if (!outFile)
            {
                throw std::runtime_error("HeaderWriter()-Error_File operation failed: " + fullOutPath.string());
            }
            CompressStrategy_uint strategySize = 0;
            CompressorVersion_uint versionSize = 0;
            HeaderOffsetSize_uint headerOffsetSize = 0;
            DirectoryOffsetSize_uint directoryOffsetSize = 0;

            numWriter.write_binary_le(outFile, strategySize);
            numWriter.write_binary_le(outFile, versionSize);
            numWriter.write_binary_le(outFile, headerOffsetSize);
            numWriter.write_binary_le(outFile, directoryOffsetSize);

            locator.offsetLocator(outFile, sizeof(MagicNum)); //定位到魔数头后才开始写，避免覆盖魔数头
            HeaderOffsetSize_uint headerOffset =locator.getFileSize(fullOutPath);
            locator.offsetLocator(outFile, headerOffset - sizeof(DirectoryOffsetSize_uint) - sizeof(HeaderOffsetSize_uint));
            outFile.write(reinterpret_cast<const char *>(&headerOffset), sizeof(HeaderOffsetSize_uint));
            locator.offsetLocator(outFile, headerOffset);
            //回填偏移量并重定位指针至回填前的位置
            outFile.close();
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("HeaderWriter()-Error_File operation failed: " + std::string(e.what()));
        }

        //文件头结束--包含魔数一共11字节
        numWriter.appendMagicStatic(fullOutPath);

        //写入目录
        Directory_FileProcessor begin;
        begin.directory_fileProcessor(filePathToScan, outPutFilePath, logicalRoot);
        //目录区结束
        //回填偏移量
        numWriter.appendMagicStatic(fullOutPath);
    }

    system("pause");
    return 0;
}