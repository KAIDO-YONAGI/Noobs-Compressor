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

// 进度回调函数类型: (当前文件名, 当前文件进度百分比, 整体进度百分比, 状态消息)
using ProgressCallback = std::function<void(const std::string &, double, double, const std::string &)>;

class CompressionLoop
{
private:
    std::string compressionFilePath;
    ProgressCallback m_progressCallback;
    Y_flib::FileSize m_totalFiles;
    Y_flib::FileSize m_processedFiles;

    // 计算总文件数
    void countTotalFiles(const std::vector<std::string> &filePathToScan, PathTransfer &transfer);

    // 报告进度
    void reportProgress(const std::filesystem::path &filename,
                        Y_flib::FileSize blockCount,
                        Y_flib::FileSize totalBlocks,
                        std::chrono::steady_clock::time_point &lastCallbackTime,
                        double &lastReportedProgress);

    // 准备下一个文件：重置 DataLoader 并更新进度相关变量
    void prepareNextFile(DataLoader *dataLoader,
                         EntryDetails &fileEntry,
                         std::filesystem::path &filename,
                         Y_flib::FileSize &totalBlocks,
                         Y_flib::FileSize &blockCount);

public:
    CompressionLoop(const std::string compressionFilePath)
        : compressionFilePath(compressionFilePath), m_progressCallback(nullptr), m_totalFiles(0), m_processedFiles(0)
    {
    }

    void setProgressCallback(ProgressCallback callback)
    {
        m_progressCallback = callback;
    }

    void compressionLoop(const std::vector<std::string> &filePathToScan,
                         Y_flib::IEncryption &encryption,
                         Y_flib::ICompression &compression,
                         Y_flib::CompressionMode mode);
};
