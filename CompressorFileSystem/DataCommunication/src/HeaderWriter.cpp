#include "../include/HeaderWriter.h"
namespace fs = std::filesystem;

void HeaderWriter_v0::writeHeader(std::ofstream &outFile,fs::path &fullOutPath)
{
    StandardWriter standardWriter;
    Locator locator;
    if (!outFile)
    {
        throw std::runtime_error("HeaderWriter()-Error_File operation failed: " + fullOutPath.string());
    }
    // 文件头
    Y_flib::CompressStrategy strategySize = STRATEGY;
    Y_flib::CompressorVersion versionSize = VERSION;
    Y_flib::HeaderOffsetSize headerOffsetSize = 0;
    Y_flib::DirectoryOffsetSize directoryOffsetSize = 0;

    standardWriter.writeBinaryStandards(strategySize,outFile);
    standardWriter.writeBinaryStandards(versionSize,outFile);
    standardWriter.writeBinaryStandards(headerOffsetSize,outFile);
    standardWriter.writeBinaryStandards(directoryOffsetSize,outFile);

    // 回填偏移量并重定位指针至回填前的位置
    locator.locateFromBegin(outFile,HEADER_SIZE - sizeof(MAGIC_NUM) - sizeof(Y_flib::DirectoryOffsetSize) - sizeof(Y_flib::HeaderOffsetSize));
    standardWriter.writeBinaryStandards(HEADER_SIZE,outFile);
    locator.locateFromEnd(outFile, 0);
}
void HeaderWriter_v0::writeDirectory(std::ofstream &outFile, const  std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot)
{

    StandardWriter standardWriter;
    Locator locator;

    Directory_FileProcessor begin(outFile);
    begin.directory_fileProcessor(filePathToScan, fullOutPath, logicalRoot);

    // 回填偏移量并重定位指针至回填前的位置
    locator.locateFromBegin(outFile, HEADER_SIZE - sizeof(MAGIC_NUM) - sizeof(Y_flib::DirectoryOffsetSize));
    Y_flib::DirectoryOffsetSize directoryOffset = locator.getFileSize(fullOutPath, outFile);
    standardWriter.writeBinaryStandards(directoryOffset + Y_flib::DirectoryOffsetSize(sizeof(MAGIC_NUM)), outFile); // sizeof(MAGIC_NUM)认为整个目录+文件头是包含末尾魔数的，只不过此时还未写入
    locator.locateFromEnd(outFile, 0);
}
void HeaderWriter::headerWriter(const std::vector<std::string> &filePathToScan, std::string &outPutFilePath, const std::string &logicalRoot)
{
    PathTransfer transfer;

    try
    {
        fs::path fullOutPath = fs::path(transfer.transPath(outPutFilePath));
        if (fs::exists(fullOutPath))
        {
            throw std::runtime_error("HeaderWriter.cpp-Error_fileIsExist\nTry to clear:" + fullOutPath.string());
        }

        std::ofstream outFile(fullOutPath, std::ios::binary | std::ios::out | std::ios::ate); // ate打开，避免覆写文件，使用偏移量定位
        if (!outFile)
        {
            throw std::runtime_error("HeaderWriter()-Error_File open failed: " + fullOutPath.string());
        }

        try
        {
            StandardWriter standardWriter;
            // 写入表示文件起始的4字节魔数
            standardWriter.appendMagicStatic(outFile);
            writeHeader(outFile,fullOutPath); // 文件头结束--包含魔数一共11字节(已回填)

            standardWriter.appendMagicStatic(outFile);

            // 目录信息
            writeDirectory(outFile, filePathToScan, fullOutPath, logicalRoot); // 目录区结束（已回填）

            standardWriter.appendMagicStatic(outFile); // 文件末尾魔数
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("HeaderWriter()-Error_File operation failed: " + std::string(e.what()));
        }

        outFile.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        throw (e.what()); // 重新抛出异常以便上层处理
    }
}