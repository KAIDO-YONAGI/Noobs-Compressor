#pragma once

#include "../CompressorFileSystem/DataCommunication/include/FileLibrary.h"
#include "../CompressorFileSystem/DataCommunication/include/DataLoader.h"
#include "../CompressorFileSystem/DataCommunication/include/DataExporter.h"
#include "../CompressorFileSystem/DataCommunication/include/BinaryStandardLoader.h"
#include "../CompressorFileSystem/DataCommunication/include/ToolClasses.h"
#include "../CompressionModules/heffman/include/Heffman.h"
#include "My_Aes.h"
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
    ProgressCallback m_progressCallback;
    Y_flib::FileSize m_totalFiles;
    Y_flib::FileSize m_processedFiles;

    // 处理目录队列
    void processDirectories(BinaryStandardLoader &headerLoaderIterator);

    // 处理单个文件
    void processFile(BinaryStandardLoader &headerLoaderIterator,
                     Aes &aes,
                     Locator &locator,
                     Y_flib::DirectoryOffsetSize &dataOffset,
                     std::chrono::steady_clock::time_point &lastCallbackTime,
                     double &lastReportedProgress);

    // 处理数据块
    void processDataBlock(std::ifstream &inFile,
                          const std::filesystem::path &filePath,
                          Aes &aes,
                          Heffman &huffmanUnzip,
                          Y_flib::DataBlock &rawTreeData,
                          Y_flib::DataBlock &decryptedTreeData,
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
        : m_progressCallback(nullptr), m_totalFiles(0), m_processedFiles(0)
    {
        PathTransfer transfer;
        fullPath = transfer.transPath(deCompressionFilePath);

        if (outputDirectory.empty() || outputDirectory == ".")
        {
            parentPath = fullPath.parent_path();
        }
        else
        {
            parentPath = std::filesystem::path(outputDirectory);
        }
    }

    void setProgressCallback(ProgressCallback callback)
    {
        m_progressCallback = callback;
    }

    void decompressionLoop(Aes &aes);
};
