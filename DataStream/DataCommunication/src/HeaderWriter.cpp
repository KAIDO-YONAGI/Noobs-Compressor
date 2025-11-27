#include "../include/HeaderWriter.h"

void HeaderWriter_v0::writeHeader(std::ofstream &outFile, fs::path &fullOutPath)
{
    MagicNumWriter numWriter;
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

    numWriter.writeBinary(outFile, strategySize);
    numWriter.writeBinary(outFile, versionSize);
    numWriter.writeBinary(outFile, headerOffsetSize);
    numWriter.writeBinary(outFile, directoryOffsetSize);

    // 回填偏移量并重定位指针至回填前的位置
    HeaderOffsetSize_uint headerOffset = locator.getFileSize(fullOutPath,outFile);
    locator.offsetLocator(outFile, HeaderSize-sizeof(MagicNum)-sizeof(DirectoryOffsetSize_uint));
    numWriter.writeBinary(outFile, headerOffset);
    outFile.seekp(0, std::ios::end);
}
void HeaderWriter_v0::writeDirectory(std::ofstream &outFile, const std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot)
{

    MagicNumWriter numWriter;
    Locator locator;

    Directory_FileProcessor begin;
    begin.directory_fileProcessor(filePathToScan, fullOutPath, logicalRoot, outFile);

    // 回填偏移量并重定位指针至回填前的位置
    locator.offsetLocator(outFile, sizeof(MagicNum) + sizeof(CompressStrategy_uint) + sizeof(CompressorVersion_uint) + sizeof(HeaderOffsetSize_uint));
    DirectoryOffsetSize_uint directoryOffset = locator.getFileSize(fullOutPath,outFile);
    numWriter.writeBinary(outFile, directoryOffset+sizeof(MagicNum));//sizeof(MagicNum)认为整个目录+文件头是包含末尾魔数的，只不过此时还未写入
    outFile.seekp(0, std::ios::end);
}
void HeaderWriter::headerWriter(std::vector<std::string> &filePathToScan, std::string &outPutFilePath, const std::string &logicalRoot)
{
    Transfer transfer;
    MagicNumWriter numWriter;
    HeaderWriter headerWriter;

    try
    {
        fs::path fullOutPath = fs::path(transfer._getPath(outPutFilePath));
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

            // 写入表示文件起始的4字节魔数
            numWriter.appendMagicStatic(outFile);
            headerWriter.writeHeader(outFile, fullOutPath); // 文件头结束--包含魔数一共11字节(已回填)

            numWriter.appendMagicStatic(outFile);

            // 目录信息
            headerWriter.writeDirectory(outFile, filePathToScan, fullOutPath, logicalRoot); // 目录区结束（已回填）

            numWriter.appendMagicStatic(outFile);
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