// HeaderLoader.h
#pragma once
#include "../CompressorFileSystem/DataCommunication/include/FileLibrary.h"
#include "../CompressorFileSystem/DataCommunication/include/DataLoader.h"
#include "../CompressorFileSystem/DataCommunication/include/DataExporter.h"
#include "../CompressorFileSystem/DataCommunication/include/BinaryIO_Loader.h"
#include "../CompressorFileSystem/DataCommunication/include/ToolClasses.h"
#include "../CompressionModules/heffman/include/Heffman.h"
#include "Aes.h"

class CompressionLoop
{
private:
    Transfer transfer;
    std::string compressionFilePath;

public:
    CompressionLoop(const std::string compressionFilePath)
    {
        this->compressionFilePath = compressionFilePath;
    }
    void compressionLoop(const std::vector<std::string> &filePathToScan, Aes &aes);
};
class DecompressionLoop
{
private:
    std::string deCompressionFilePath;
    fs::path rootPath;
    fs::path deCompressionFullPath;
    std::ifstream deCompressionFile;
    void createDirectory(const fs::path &path);
    // 创建文件 (创建空文件)
    void createFile(const fs::path &filePath);

public:
    DecompressionLoop(const std::string deCompressionFilePath)
    {
        Transfer transfer;
        rootPath = transfer.transPath(deCompressionFilePath).parent_path();
        this->deCompressionFilePath = deCompressionFilePath;
        this->deCompressionFullPath = transfer.transPath(deCompressionFilePath);
        this->deCompressionFile = std::move(std::ifstream(deCompressionFullPath,std::ios::binary));
    }
    void decompressionLoop(Aes &aes);
};