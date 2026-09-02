#pragma once

#include "../CompressorFileSystem/Commons/include/FileLibrary.h"
#include "../CompressorFileSystem/DataIO/include/DataLoader.h"
#include "../CompressorFileSystem/DataIO/include/DataExporter.h"
#include "../CompressorFileSystem/ArchiveFormat/include/BinaryStandardLoader.h"
#include "../CompressorFileSystem/Commons/include/ToolClasses.h"
#include "../CompressorFileSystem/Strategy/include/ICompression.h"
#include "../CompressorFileSystem/Strategy/include/IEncryption.h"
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
    ProgressCallback progressCallback;
    Y_flib::FileSize totalFiles;
    Y_flib::FileSize processedFiles;

    // 计算总文件数
    void countTotalFiles(const std::vector<std::string> &filePathToScan);

    // 报告进度
    void reportProgress(const std::filesystem::path &filename,
                        Y_flib::FileSize blockCount,
                        Y_flib::FileSize totalBlocks,
                        std::chrono::steady_clock::time_point &lastCallbackTime,
                        double &lastReportedProgress);

    // 准备下一个文件：重置 DataLoader 并更新进度相关变量
    void prepareNextFile(Y_flib::DataLoader *dataLoader,
                         Y_flib::EntryDetails &fileEntry,
                         std::filesystem::path &filename,
                         Y_flib::FileSize &totalBlocks,
                         Y_flib::FileSize &blockCount);

public:
    CompressionLoop(const std::string compressionFilePath)
        : compressionFilePath(compressionFilePath), progressCallback(nullptr), totalFiles(0), processedFiles(0)
    {
    }

    void setProgressCallback(ProgressCallback callback)
    {
        progressCallback = callback;
    }

    void compressionLoop(const std::vector<std::string> &filePathToScan,
                         Y_flib::IEncryption &encryption,
                         Y_flib::ICompression &compression,
                         Y_flib::CompressionMode mode);
};
