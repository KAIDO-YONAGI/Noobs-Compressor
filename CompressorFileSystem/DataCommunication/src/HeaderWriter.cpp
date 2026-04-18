#include "../include/HeaderWriter.h"
#include "../include/StrategyFactory.h"

namespace Y_flib
{
void HeaderWriter_v0::writeHeader(std::ofstream &outFile, std::filesystem::path &fullOutPath, Y_flib::CompressionMode mode)
{
    StandardsWriter standardWriter;
    Locator locator;
    if (!outFile)
    {
        throw std::runtime_error("HeaderWriter()-Error_File operation failed: " + EncodingUtils::pathToUtf8(fullOutPath));
    }
    // 文件头 - 使用传入的策略模式
    Y_flib::CompressStrategy strategySize = Y_flib::StrategyFactory::modeToId(mode);
    Y_flib::CompressorVersion versionSize = Y_flib::Constants::VERSION;
    Y_flib::HeaderOffsetSize headerOffsetSize = 0;
    Y_flib::DirectoryOffsetSize directoryOffsetSize = 0;

    standardWriter.writeBinaryStandards(strategySize,outFile);
    standardWriter.writeBinaryStandards(versionSize,outFile);
    standardWriter.writeBinaryStandards(headerOffsetSize,outFile);
    standardWriter.writeBinaryStandards(directoryOffsetSize,outFile);

    // 回填偏移量并重定位指针至回填前的位置
    locator.locateFromBegin(outFile,Y_flib::Constants::HEADER_SIZE - sizeof(Y_flib::Constants::MAGIC_NUM) - sizeof(Y_flib::DirectoryOffsetSize) - sizeof(Y_flib::HeaderOffsetSize));
    standardWriter.writeBinaryStandards(Y_flib::Constants::HEADER_SIZE,outFile);
    locator.locateFromEnd(outFile, 0);
}
void HeaderWriter_v0::writeDirectory(std::ofstream &outFile, const  std::vector<std::string> &filePathToScan, const std::filesystem::path &fullOutPath, const std::string &logicalRoot)
{

    StandardsWriter standardWriter;
    Locator locator;

    EntryProcessor begin(outFile);
    begin.entryProcessor(filePathToScan, fullOutPath, logicalRoot);

    // 先获取文件大小（在移动指针之前）
    outFile.flush();
    std::streampos endPos = outFile.tellp();
    Y_flib::DirectoryOffsetSize directoryOffset = static_cast<Y_flib::DirectoryOffsetSize>(endPos);
    std::cout << "DEBUG writeDirectory: fullOutPath=" << EncodingUtils::pathToUtf8(fullOutPath) << ", directoryOffset=" << directoryOffset << std::endl;

    // 回填偏移量并重定位指针至回填前的位置
    locator.locateFromBegin(outFile, Y_flib::Constants::HEADER_SIZE - sizeof(Y_flib::Constants::MAGIC_NUM) - sizeof(Y_flib::DirectoryOffsetSize));
    standardWriter.writeBinaryStandards(directoryOffset + Y_flib::DirectoryOffsetSize(sizeof(Y_flib::Constants::MAGIC_NUM)), outFile); // sizeof(MAGIC_NUM)认为整个目录+文件头是包含末尾魔数的，只不过此时还未写入
    locator.locateFromEnd(outFile, 0);
}
void HeaderWriter::headerWriter(const std::vector<std::string> &filePathToScan, std::string &outputFilePath, const std::string &logicalRoot, Y_flib::CompressionMode mode)
{
    try
    {
        std::filesystem::path fullOutPath = EncodingUtils::pathFromUtf8(outputFilePath);
        if (std::filesystem::exists(fullOutPath))
        {
            throw std::runtime_error("HeaderWriter.cpp-Error_fileIsExist\nTry to clear:" + EncodingUtils::pathToUtf8(fullOutPath));
        }

        std::ofstream outFile(fullOutPath, std::ios::binary | std::ios::out | std::ios::ate); // ate打开，避免覆写文件，使用偏移量定位
        if (!outFile)
        {
            throw std::runtime_error("HeaderWriter()-Error_File open failed: " + EncodingUtils::pathToUtf8(fullOutPath));
        }

        try
        {
            StandardsWriter standardWriter;
            // 写入表示文件起始的4字节魔数
            standardWriter.appendMagicStatic(outFile);
            writeHeader(outFile,fullOutPath, mode); // 文件头结束--包含魔数一共11字节(已回填)

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
        throw std::runtime_error(std::string("HeaderWriter encountered an error: ") + e.what());
    }
}
} // namespace Y_flib
