#include "CompressionLoop.h"
#include <chrono>

// 进度回调最小间隔（毫秒）
static constexpr int PROGRESS_CALLBACK_INTERVAL_MS = 100;

void CompressionLoop::compressionLoop(const std::vector<std::string> &filePathToScan, Aes &aes)
{
    // 初始化迭代器
    std::filesystem::path blank;
    BinaryStandardLoader headerLoaderIterator(compressionFilePath, filePathToScan, blank);

    Heffman huffmanZip(1);

    PathTransfer transfer;

    Y_flib::DataBlock encryptedBlock;

    std::filesystem::path loadPath;
    std::unique_ptr<DataLoader> dataLoader;

    Y_flib::FileSize totalBlocks = 1, blockCount = 0;

    // 进度回调节流
    auto lastCallbackTime = std::chrono::steady_clock::now();
    double lastReportedProgress = -1.0;

    // 计算总文件数用于进度报告
    countTotalFiles(filePathToScan, transfer);

    headerLoaderIterator.headerLoaderIterator(aes); // 执行第一次操作，把根目录载入
    if (!headerLoaderIterator.fileQueue.empty())    // 单个文件特殊处理
    {
        EntryDetails loadFile = headerLoaderIterator.fileQueue.front().first;
        loadPath = loadFile.getFullPath();
        dataLoader = std::make_unique<DataLoader>(loadPath);
        totalBlocks = (loadFile.getFileSizeInDetails() + Y_flib::Constants::BUFFER_SIZE - 1) / Y_flib::Constants::BUFFER_SIZE;
    }

    DataExporter dataExporter(transfer.transPath(compressionFilePath));

    // 预分配缓冲区，在循环中复用，避免频繁内存分配
    Y_flib::DataBlock huffTree;
    Y_flib::DataBlock huffTreeOutPut;
    Y_flib::DataBlock compressedData;
    huffTree.reserve(Y_flib::Constants::BUFFER_SIZE);
    huffTreeOutPut.reserve(Y_flib::Constants::BUFFER_SIZE);
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

            huffmanZip.statistic_freq(0, data_In);
            huffmanZip.merge_ttabs();
            huffmanZip.gen_hefftree();
            huffmanZip.save_code_inTab();

            huffTree.clear();
            huffmanZip.tree_to_plat_uchar(huffTree);
            huffTreeOutPut.clear();
            huffTreeOutPut.resize(huffTree.size());

            aes.doAes(1, huffTree, huffTreeOutPut);
            dataExporter.exportCompressedData(huffTreeOutPut);

            compressedData.clear();
            huffmanZip.encode(data_In, compressedData);

            encryptedBlock.clear();
            aes.doAes(1, compressedData, encryptedBlock);

            dataExporter.exportCompressedData(encryptedBlock);

            // 计算进度并回调
            reportProgress(filename, blockCount, totalBlocks, lastCallbackTime, lastReportedProgress);
        }

        if (dataLoader->isDone() && !headerLoaderIterator.fileQueue.empty()) // 当前文件处理完成，准备下一个文件
        {
            Y_flib::FileNameSize offsetToFill = headerLoaderIterator.fileQueue.front().second;
            dataExporter.thisFileIsDone(offsetToFill);

            headerLoaderIterator.fileQueue.pop();
            m_processedFiles++;

            if (!headerLoaderIterator.fileQueue.empty())
            {
                EntryDetails newLoadFile = headerLoaderIterator.fileQueue.front().first;
                dataLoader->reset(newLoadFile.getFullPath());
                filename = newLoadFile.getFullPath().filename();
                totalBlocks = (newLoadFile.getFileSizeInDetails() + Y_flib::Constants::BUFFER_SIZE - 1) / Y_flib::Constants::BUFFER_SIZE;
                blockCount = 0;
            }
        }

        while (headerLoaderIterator.fileQueue.empty() && !headerLoaderIterator.allLoopIsDone())
        {
            headerLoaderIterator.restartLoader();
            headerLoaderIterator.headerLoaderIterator(aes);

            if (!headerLoaderIterator.fileQueue.empty())
            {
                EntryDetails newLoadFile = headerLoaderIterator.fileQueue.front().first;
                dataLoader->reset(newLoadFile.getFullPath());
                filename = newLoadFile.getFullPath().filename();
                totalBlocks = (newLoadFile.getFileSizeInDetails() + Y_flib::Constants::BUFFER_SIZE - 1) / Y_flib::Constants::BUFFER_SIZE;
                blockCount = 0;
            }
        }
    }
    headerLoaderIterator.encryptHeaderBlock(aes);

    // 完成回调
    if (m_progressCallback)
    {
        m_progressCallback("", 100.0, 100.0, "Completed");
    }
}

void CompressionLoop::countTotalFiles(const std::vector<std::string> &filePathToScan, PathTransfer &transfer)
{
    m_totalFiles = 0;
    m_processedFiles = 0;
    for (const auto &path : filePathToScan)
    {
        try
        {
            std::filesystem::path fsPath = transfer.transPath(path);
            if (std::filesystem::is_directory(fsPath))
            {
                for (auto it = std::filesystem::recursive_directory_iterator(fsPath);
                     it != std::filesystem::recursive_directory_iterator(); ++it)
                {
                    if (it->is_regular_file())
                        m_totalFiles++;
                }
            }
            else if (std::filesystem::is_regular_file(fsPath))
            {
                m_totalFiles++;
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

void CompressionLoop::reportProgress(const std::filesystem::path &filename,
                                     Y_flib::FileSize blockCount,
                                     Y_flib::FileSize totalBlocks,
                                     std::chrono::steady_clock::time_point &lastCallbackTime,
                                     double &lastReportedProgress)
{
    double fileProgress = (100.0 * blockCount) / totalBlocks;
    double overallProgress = m_totalFiles > 0 ? (100.0 * m_processedFiles / m_totalFiles) : 0;
    if (m_totalFiles > 0)
    {
        overallProgress = 100.0 * (m_processedFiles + fileProgress / 100.0) / m_totalFiles;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCallbackTime).count();
    bool shouldReport = (elapsed >= PROGRESS_CALLBACK_INTERVAL_MS) ||
                        (overallProgress - lastReportedProgress >= 5.0) ||
                        (blockCount == totalBlocks);

    if (m_progressCallback && shouldReport)
    {
        m_progressCallback(filename.string(), fileProgress, overallProgress, "Compressing");
        lastCallbackTime = now;
        lastReportedProgress = overallProgress;
    }
}
