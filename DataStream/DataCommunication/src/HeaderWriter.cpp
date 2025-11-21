#include "../include/Directory_FileProcessor.h"
int main()
{
    std::vector<std::string> filePathToScan;
    std::string outPutFilePath, logicalRoot;
    MagicNumWriter numWriter;
    Transfer transfer;
    Locator locator;

    //假装是gui获取的
    filePathToScan.push_back("D:\\1gal");
    filePathToScan.push_back("D:\\1gal\\TEST\\我挚爱的时光");
    filePathToScan.push_back("D:\\1gal\\TEST\\123意514.txt");
    outPutFilePath = "挚爱的时光.bin";
    logicalRoot = "YONAGI";


    try
    {   fs::path fullOutPath = fs::path(transfer._getPath(outPutFilePath));
        if (fs::exists(fullOutPath))
        {
            throw std::runtime_error("HeaderWriter.cpp(main())-Error_fileIsExist\nTry to clear:" + fullOutPath.string());
        }

        std::ofstream outFile(fullOutPath, std::ios::binary | std::ios::out | std::ios::ate);
        if (!outFile)
        {
            throw std::runtime_error("HeaderWriter()-Error_File open failed: " + fullOutPath.string());
        }

        try
        {

            //写入表示文件起始的4字节魔数
            numWriter.appendMagicStatic(outFile);
            if (!outFile)
            {
                throw std::runtime_error("HeaderWriter()-Error_File operation failed: " + fullOutPath.string());
            }
            //文件头
            CompressStrategy_uint strategySize = 0;
            CompressorVersion_uint versionSize = 0;
            HeaderOffsetSize_uint headerOffsetSize = 0;
            DirectoryOffsetSize_uint directoryOffsetSize = 0;

            numWriter.write_binary_le(outFile, strategySize);
            numWriter.write_binary_le(outFile, versionSize);
            numWriter.write_binary_le(outFile, headerOffsetSize);
            numWriter.write_binary_le(outFile, directoryOffsetSize);

            //回填偏移量并重定位指针至回填前的位置
            locator.offsetLocator(outFile, sizeof(MagicNum)); //定位到魔数头后才开始写，避免覆盖魔数头
            HeaderOffsetSize_uint headerOffset = locator.getFileSize(fullOutPath);
            locator.offsetLocator(outFile, headerOffset - sizeof(DirectoryOffsetSize_uint) - sizeof(HeaderOffsetSize_uint));
            numWriter.write_binary_le(outFile, headerOffset);
            outFile.seekp(0, std::ios::end);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("HeaderWriter()-Error_File operation failed: " + std::string(e.what()));
        }

        //文件头结束--包含魔数一共11字节
        numWriter.appendMagicStatic(outFile);

        //目录信息
        Directory_FileProcessor begin;
        begin.directory_fileProcessor(filePathToScan, outPutFilePath, logicalRoot, outFile);
        //目录区结束
        numWriter.appendMagicStatic(outFile);
        //回填偏移量并重定位指针至回填前的位置
        locator.offsetLocator(outFile, sizeof(MagicNum) + sizeof(CompressStrategy_uint) + sizeof(CompressorVersion_uint) + sizeof(HeaderOffsetSize_uint));
        DirectoryOffsetSize_uint directoryOffset = locator.getFileSize(fullOutPath);
        numWriter.write_binary_le(outFile, directoryOffset);
        outFile.seekp(0, std::ios::end);

        outFile.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        throw; // 重新抛出异常以便上层处理
    }

    system("pause");
    return 0;
}