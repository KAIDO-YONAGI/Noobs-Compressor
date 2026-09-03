#include "CompressionLoop.h"
#include "../CompressorFileSystem/Commons/include/FileSystemUtils.h"
#include <chrono>
#include <memory>
#include <vector>

using Y_flib::BinaryStandardLoader;
using Y_flib::DataExporter;
using Y_flib::DataLoader;
using Y_flib::EncodingUtils;
using Y_flib::EntryDetails;

// 进度回调最小间隔（毫秒）
static constexpr int PROGRESS_CALLBACK_INTERVAL_MS = 100;

void CompressionLoop::compressionLoop(
    const std::vector<std::string> &filePathToScan,
    Y_flib::IEncryption &encryption,
    Y_flib::ICompression &compression,
    Y_flib::CompressionMode mode)
{
    // 初始化迭代器
    std::filesystem::path blank;
    BinaryStandardLoader headerLoaderIterator(compressionFilePath, filePathToScan, blank);

    Y_flib::DataBlock encryptedBlock;

    std::filesystem::path loadPath;
    std::unique_ptr<DataLoader> dataLoader;

    Y_flib::FileSize totalBlocks = 1, blockCount = 0;

    // 进度回调节流
    auto lastCallbackTime = std::chrono::steady_clock::now();
    double lastReportedProgress = -1.0;

    // 计算总文件数用于进度报告
    countTotalFiles(filePathToScan);

    headerLoaderIterator.headerLoaderIterator(encryption); // 执行第一次操作，把根目录载入
    // 首个目录块可能只有目录条目而没有文件条目（BFS 层序 + 16KB 分割所致），
    // 需持续拉块直到出现文件条目或目录读取完成，否则主循环会因队列空而静默跳过
    while (headerLoaderIterator.fileQueue.empty() && !headerLoaderIterator.allLoopIsDone())
    {
        headerLoaderIterator.restartLoader();
        headerLoaderIterator.headerLoaderIterator(encryption);
    }
    if (!headerLoaderIterator.fileQueue.empty()) // 单个文件特殊处理
    {
        EntryDetails loadFile = headerLoaderIterator.fileQueue.front().entry;
        loadPath = loadFile.getFullPath();
        dataLoader = std::make_unique<DataLoader>(loadPath);
        totalBlocks = (loadFile.getFileSizeInDetails() + Y_flib::Constants::BUFFER_SIZE - 1) / Y_flib::Constants::BUFFER_SIZE;
    }

    DataExporter dataExporter(EncodingUtils::pathFromUtf8(compressionFilePath));

    // 预分配缓冲区，在循环中复用，避免频繁内存分配
    Y_flib::DataBlock metadata;
    Y_flib::DataBlock compressedData;
    metadata.reserve(Y_flib::Constants::BUFFER_SIZE);
    compressedData.reserve(Y_flib::Constants::BUFFER_SIZE);
    encryptedBlock.reserve(Y_flib::Constants::BUFFER_SIZE * 2);

    std::filesystem::path filename = loadPath.filename();
    while (!headerLoaderIterator.fileQueue.empty())
    {
        dataLoader->dataLoader();

        if ((!dataLoader->isDone() && blockCount < totalBlocks)) // 处理当前文件的每个数据块
        {
            blockCount++;
            const Y_flib::DataBlock data_In = dataLoader->getBlock();

            // 通过接口调用压缩模块
            metadata.clear();
            compressedData.clear();
            compression.compress(data_In, metadata, compressedData);

            encryption.encrypt(metadata, encryptedBlock);
            dataExporter.exportCompressedData(encryptedBlock);

            encryptedBlock.clear();
            encryption.encrypt(compressedData, encryptedBlock);
            dataExporter.exportCompressedData(encryptedBlock);
            //TODO:这可以把两次io优化为一次 方法是合并两个数据块后再写（因为逻辑上也是连续的）

            // 计算进度并回调
            reportProgress(filename, blockCount, totalBlocks, lastCallbackTime, lastReportedProgress);
        }

        if (dataLoader->isDone() && !headerLoaderIterator.fileQueue.empty()) // 当前文件处理完成，准备下一个文件
        {
            Y_flib::SlotOffset offsetToFill = headerLoaderIterator.fileQueue.front().processedSizeOffset;
            dataExporter.thisFileIsDone(offsetToFill);

            headerLoaderIterator.fileQueue.pop();
            processedFiles++;

            if (!headerLoaderIterator.fileQueue.empty())
            {
                prepareNextFile(dataLoader.get(), headerLoaderIterator.fileQueue.front().entry,
                                filename, totalBlocks, blockCount);
            }
        }

        while (headerLoaderIterator.fileQueue.empty() && !headerLoaderIterator.allLoopIsDone())
        {
            headerLoaderIterator.restartLoader();
            headerLoaderIterator.headerLoaderIterator(encryption);

            if (!headerLoaderIterator.fileQueue.empty())
            {
                prepareNextFile(dataLoader.get(), headerLoaderIterator.fileQueue.front().entry,
                                filename, totalBlocks, blockCount);
            }
        }
    }
    headerLoaderIterator.encryptHeaderBlock(encryption, mode);

    // 完成回调
    if (progressCallback)
    {
        progressCallback("", 100.0, 100.0, "Completed");
    }
}

void CompressionLoop::countTotalFiles(const std::vector<std::string> &filePathToScan)
{
    totalFiles = 0;
    processedFiles = 0;
    for (const auto &path : filePathToScan)
    {
        try
        {
            std::filesystem::path fsPath = EncodingUtils::pathFromUtf8(path);
            const Y_flib::FileSystemEntryInfo rootInfo =
                Y_flib::FileSystemUtils::queryEntry(fsPath);
            if (rootInfo.isDirectory)
            {
                std::vector<std::filesystem::path> directories{fsPath};
                while (!directories.empty())
                {
                    const std::filesystem::path directory = directories.back();
                    directories.pop_back();

                    for (const std::filesystem::path &fullPath :
                         Y_flib::FileSystemUtils::listDirectory(directory))
                    {
                        const Y_flib::FileSystemEntryInfo info =
                            Y_flib::FileSystemUtils::queryEntry(fullPath);
                        if (info.isRegularFile)
                        {
                            ++totalFiles;
                        }
                        else if (info.isDirectory)
                        {
                            directories.push_back(fullPath);
                        }
                    }
                }
            }
            else if (rootInfo.isRegularFile)
            {
                totalFiles++;
            }
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("Cannot access path: " + path + " - " + e.what());
        }
        catch (...)
        {
            throw std::runtime_error("Cannot access path: " + path + " - Unknown error");
        }
    }
}

void CompressionLoop::reportProgress(
    const std::filesystem::path &filename,
    Y_flib::FileSize blockCount,
    Y_flib::FileSize totalBlocks,
    std::chrono::steady_clock::time_point &lastCallbackTime,
    double &lastReportedProgress)
{
    double fileProgress = (100.0 * blockCount) / totalBlocks;
    double overallProgress = totalFiles > 0 ? (100.0 * processedFiles / totalFiles) : 0;
    if (totalFiles > 0)
    {
        overallProgress = 100.0 * (processedFiles + fileProgress / 100.0) / totalFiles;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCallbackTime).count();
    bool shouldReport = (elapsed >= PROGRESS_CALLBACK_INTERVAL_MS) ||
                        (overallProgress - lastReportedProgress >= 5.0) ||
                        (blockCount == totalBlocks);

    if (progressCallback && shouldReport)
    {
        progressCallback(EncodingUtils::pathToUtf8(filename), fileProgress, overallProgress, "Compressing");
        lastCallbackTime = now;
        lastReportedProgress = overallProgress;
    }
}

void CompressionLoop::prepareNextFile(
    Y_flib::DataLoader *dataLoader,
    Y_flib::EntryDetails &fileEntry,
    std::filesystem::path &filename,
    Y_flib::FileSize &totalBlocks,
    Y_flib::FileSize &blockCount)
{
    dataLoader->reset(fileEntry.getFullPath());
    filename = fileEntry.getFullPath().filename();
    totalBlocks = (fileEntry.getFileSizeInDetails() + Y_flib::Constants::BUFFER_SIZE - 1) / Y_flib::Constants::BUFFER_SIZE;
    blockCount = 0;
}
