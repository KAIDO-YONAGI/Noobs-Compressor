#pragma once
#include "../CompressorFileSystem/DataCommunication/include/FileLibrary.h"
#include "../CompressorFileSystem/DataCommunication/include/DataLoader.h"
#include "../CompressorFileSystem/DataCommunication/include/DataExporter.h"
#include "../CompressorFileSystem/DataCommunication/include/BinaryStandardLoader.h"
#include "../CompressorFileSystem/DataCommunication/include/ToolClasses.h"
#include "../CompressionModules/heffman/include/Heffman.h"
#include "My_Aes.h"
#include <filesystem>
class CompressionLoop
{ 
private:
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
    
    std::filesystem::path parentPath;
    std::filesystem::path fullPath;
    void createDirectory(const std::filesystem::path &path);
    // 创建文件 (创建空文件)
    bool createFile(const std::filesystem::path &filePath);

public:
    DecompressionLoop(std::string deCompressionFilePath, std::string outputDirectory = "")
    {
        PathTransfer transfer;
        fullPath = transfer.transPath(deCompressionFilePath);

        if (outputDirectory.empty() || outputDirectory == ".")
        {
            // 默认行为：使用压缩文件所在目录
            parentPath = fullPath.parent_path();
        }
        else
        {
            // 使用指定的输出目录（outputDirectory 已是绝对路径）
            parentPath = std::filesystem::path(outputDirectory);
        }
    }
    void decompressionLoop(Aes &aes);
};