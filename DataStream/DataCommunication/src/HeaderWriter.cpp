#include "../include/HeaderWriter.h"

void HeaderWriter_v0::writeHeader(std::ofstream &outFile,fs::path &fullOutPath)
{
    NumsWriter numWriter(outFile);
    Locator locator;
    if (!outFile)
    {
        throw std::runtime_error("HeaderWriter()-Error_File operation failed: " + fullOutPath.string());
    }
    // 文件头
    CompressStrategy_uint strategySize = 0;
    CompressorVersion_uint versionSize = 0;
    HeaderOffsetSize_uint headerOffsetSize = 0;
    DirectoryOffsetSize_uint directoryOffsetSize = 0;

    numWriter.writeBinaryNums(strategySize);
    numWriter.writeBinaryNums(versionSize);
    numWriter.writeBinaryNums(headerOffsetSize);
    numWriter.writeBinaryNums(directoryOffsetSize);

    // 回填偏移量并重定位指针至回填前的位置
    locator.offsetLocator(outFile,HeaderSize - sizeof(MagicNum) - sizeof(DirectoryOffsetSize_uint) - sizeof(headerOffsetSize));
    numWriter.writeBinaryNums(HeaderSize);
    outFile.seekp(0, std::ios::end);
}
void HeaderWriter_v0::writeDirectory(std::ofstream &outFile, const std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot)
{

    NumsWriter numWriter(outFile);
    Locator locator;

    Directory_FileProcessor begin(outFile);
    begin.directory_fileProcessor(filePathToScan, fullOutPath, logicalRoot);

    // 回填偏移量并重定位指针至回填前的位置
    locator.offsetLocator(outFile, HeaderSize - sizeof(MagicNum) - sizeof(DirectoryOffsetSize_uint));
    DirectoryOffsetSize_uint directoryOffset = locator.getFileSize(fullOutPath, outFile);
    numWriter.writeBinaryNums(directoryOffset + DirectoryOffsetSize_uint(sizeof(MagicNum))); // sizeof(MagicNum)认为整个目录+文件头是包含末尾魔数的，只不过此时还未写入
    outFile.seekp(0, std::ios::end);
}
void HeaderWriter::headerWriter(std::vector<std::string> &filePathToScan, std::string &outPutFilePath, const std::string &logicalRoot)
{
    Transfer transfer;

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
            NumsWriter numWriter(outFile);
            // 写入表示文件起始的4字节魔数
            numWriter.appendMagicStatic();
            writeHeader(outFile,fullOutPath); // 文件头结束--包含魔数一共11字节(已回填)

            numWriter.appendMagicStatic();

            // 目录信息
            writeDirectory(outFile, filePathToScan, fullOutPath, logicalRoot); // 目录区结束（已回填）

            numWriter.appendMagicStatic();
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
        throw; // 重新抛出异常以便上层处理
    }
}