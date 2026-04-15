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

// 进度回调函数类型: (当前文件名, 当前文件进度百分比, 整体进度百分比, 状态消息)
using ProgressCallback = std::function<void(const std::string&, double, double, const std::string&)>;

class CompressionLoop
{
private:
    std::string compressionFilePath;
    ProgressCallback m_progressCallback;
    Y_flib::FileSize m_totalFiles;
    Y_flib::FileSize m_processedFiles;

public:
    CompressionLoop(const std::string compressionFilePath)
        : compressionFilePath(compressionFilePath)
        , m_progressCallback(nullptr)
        , m_totalFiles(0)
        , m_processedFiles(0)
    {
    }

    void setProgressCallback(ProgressCallback callback)
    {
        m_progressCallback = callback;
    }

    void compressionLoop(const std::vector<std::string> &filePathToScan, Aes &aes);
};

class DecompressionLoop
{
private:
    std::filesystem::path parentPath;
    std::filesystem::path fullPath;
    ProgressCallback m_progressCallback;
    Y_flib::FileSize m_totalFiles;
    Y_flib::FileSize m_processedFiles;
    void createDirectory(const std::filesystem::path &path);
    bool createFile(const std::filesystem::path &filePath);

public:
    DecompressionLoop(std::string deCompressionFilePath, std::string outputDirectory = "")
        : m_progressCallback(nullptr)
        , m_totalFiles(0)
        , m_processedFiles(0)
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