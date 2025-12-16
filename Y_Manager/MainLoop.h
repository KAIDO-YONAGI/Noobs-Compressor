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
    
    fs::path parentPath;
    fs::path fullPath;
    void createDirectory(const fs::path &path);
    // 创建文件 (创建空文件)
    void createFile(const fs::path &filePath);

public:
    DecompressionLoop(std::string deCompressionFilePath)
    {
        Transfer transfer;
        fullPath=transfer.transPath(deCompressionFilePath);
        parentPath = fullPath.parent_path();

    }
    void decompressionLoop(Aes &aes);
};