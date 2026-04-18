#pragma once

#include "../CompressorFileSystem/DataCommunication/include/FileLibrary.h"
#include "../CompressorFileSystem/DataCommunication/include/DataLoader.h"
#include "../CompressorFileSystem/DataCommunication/include/DataExporter.h"
#include "../CompressorFileSystem/DataCommunication/include/BinaryStandardLoader.h"
#include "../CompressorFileSystem/DataCommunication/include/ToolClasses.h"
#include "../CompressorFileSystem/DataCommunication/include/ICompression.h"
#include "../CompressorFileSystem/DataCommunication/include/IEncryption.h"
#include <filesystem>
#include <functional>
#include <string>
#include <chrono>
#include <memory>

// 进度回调函数类型: (当前文件名, 当前文件进度百分比, 整体进度百分比, 状态消息)
using ProgressCallback = std::function<void(const std::string &, double, double, const std::string &)>;

class DecompressionLoop
{
private:
    std::filesystem::path parentPath;
    std::filesystem::path fullPath;
    ProgressCallback progressCallback;
    Y_flib::FileSize totalFiles;
    Y_flib::FileSize processedFiles;

    Y_flib::IEncryption *m_encryption = nullptr;
    Y_flib::ICompression *m_compression = nullptr;

    // 处理目录队列
    void processDirectories(BinaryStandardLoader &headerLoaderIterator);

    // 处理单个文件
    void processFile(BinaryStandardLoader &headerLoaderIterator,
                     Locator &locator,
                     Y_flib::DirectoryOffsetSize &dataOffset,
                     std::chrono::steady_clock::time_point &lastCallbackTime,
                     double &lastReportedProgress);

    // 处理数据块
    void processDataBlock(std::ifstream &inFile,
                          const std::filesystem::path &filePath,
                          Y_flib::IEncryption &encryption,
                          Y_flib::ICompression &compression,
                          Y_flib::DataBlock &rawMetadata,
                          Y_flib::DataBlock &decryptedMetadata,
                          Y_flib::DataBlock &rawData,
                          Y_flib::DataBlock &decryptedData,
                          Y_flib::DataBlock &decompressedData,
                          Y_flib::FileSize &fileCompressedSize,
                          Y_flib::FileSize &totalDecompressedBytes,
                          Y_flib::FileSize originalSize,
                          DataExporter &dataExporter);

    // 报告进度
    void reportProgress(const std::filesystem::path &filename,
                        Y_flib::FileSize totalDecompressedBytes,
                        Y_flib::FileSize originalSize,
                        std::chrono::steady_clock::time_point &lastCallbackTime,
                        double &lastReportedProgress);

    // 创建目录
    void createDirectory(const std::filesystem::path &path);

    // 创建文件
    bool createFile(const std::filesystem::path &filePath);

public:
    DecompressionLoop(std::string deCompressionFilePath, std::string outputDirectory = "")
        : progressCallback(nullptr), totalFiles(0), processedFiles(0)
    {
        PathTransfer transfer;
        fullPath = transfer.transPath(deCompressionFilePath);

        if (outputDirectory.empty() || outputDirectory == ".")
        {
            parentPath = fullPath.parent_path();
        }
        else
        {
            parentPath = transfer.transPath(outputDirectory);
        }
    }

    void setProgressCallback(ProgressCallback callback)
    {
        progressCallback = callback;
    }

    void decompressionLoop(Y_flib::IEncryption &encryption, Y_flib::ICompression &compression);
};
