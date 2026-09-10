#include "DecompressionLoop.h"
#include "../CompressorFileSystem/Commons/include/FileSystemUtils.h"
#include "../CompressorFileSystem/Strategy/include/StrategyFactory.h"
#include <algorithm>
#include <chrono>
#include <memory>
#include <utility>

using Y_flib::BinaryStandardLoader;
using Y_flib::BufferPool;
using Y_flib::DataExporter;
using Y_flib::EncodingUtils;
using Y_flib::FileTask;
using Y_flib::LinkTask;
using Y_flib::Locator;
using Y_flib::StandardsReader;

// 进度回调最小间隔（毫秒）
static constexpr int PROGRESS_CALLBACK_INTERVAL_MS = 100;

// 计算工人数量：物理并行度，取不到时退回 4（与压缩侧一致）
static unsigned pipelineWorkerCount()
{
    const unsigned count = std::thread::hardware_concurrency();
    return count == 0 ? 4u : count;
}

void DecompressionLoop::decompressionLoop(Y_flib::CompressionMode mode, const std::string &password)
{
    const unsigned workerCount = pipelineWorkerCount();
    // 池容量/块大小与压缩侧同口径：在途上限 2×(N+2)，块容量 8MB+1K（解压输出
    // 每块 ≤8MB，密文输入 ≤8MB+IV，均在容量内）
    BufferPool bufferPool(2u * (workerCount + 2u),
                          static_cast<std::size_t>(Y_flib::Constants::BUFFER_SIZE) + 1024);

    SafeQueue<DecompressTask> taskQueue;
    SafeQueue<DecompressionMessage> resultQueue;

    // 各线程的产出与异常出口，join 之后由总指挥取用
    std::queue<LinkTask> linkTasks;           // 读线程移交的链接任务（收尾重建）
    std::exception_ptr readerException = nullptr;
    std::exception_ptr writerException = nullptr;

    totalFiles = 0;
    processedFiles = 0;

    // 启动顺序：写 → 工人 → 读（读线程最先结束，逐级收口）
    std::thread writerThread(&DecompressionLoop::writerLoop, this,
                             std::ref(resultQueue), std::ref(bufferPool),
                             std::ref(writerException));
    std::vector<std::thread> workerThreads;
    workerThreads.reserve(workerCount);
    for (unsigned workerIndex = 0; workerIndex < workerCount; ++workerIndex)
        workerThreads.emplace_back(&DecompressionLoop::workerLoop, this,
                                   std::ref(taskQueue), std::ref(resultQueue),
                                   mode, password);
    std::thread readerThread(&DecompressionLoop::readerLoop, this,
                             std::ref(taskQueue), std::ref(resultQueue),
                             std::ref(bufferPool), mode, password,
                             std::ref(linkTasks), std::ref(readerException));

    // 集中收口（与压缩侧同构）：读自然结束 → 关任务队列 → 工人排空退出
    // → 关结果队列 → 写排空退出。顺序反了会丢尾块或让某端永久等待
    readerThread.join();
    taskQueue.setDone();
    for (std::thread &worker : workerThreads)
        worker.join();
    resultQueue.setDone();
    writerThread.join();

    if (readerException)
    {
        bufferPool.close();
        std::rethrow_exception(readerException);
    }
    if (writerException)
    {
        bufferPool.close();
        std::rethrow_exception(writerException);
    }

    // 链接严格最后：目标可能是刚还原的文件/目录
    processLinks(linkTasks);

    bufferPool.close();

    processDirectories(headerLoaderIterator);
    processLinks(headerLoaderIterator);

    // 完成回调
    if (progressCallback)
    {
        progressCallback("", 100.0, 100.0, "Completed");
    }
}

void DecompressionLoop::readerLoop(
    SafeQueue<DecompressTask> &taskQueue,
    SafeQueue<DecompressionMessage> &resultQueue,
    BufferPool &bufferPool,
    Y_flib::CompressionMode mode,
    const std::string &password,
    std::queue<LinkTask> &linkTasksOut,
    std::exception_ptr &exceptionOut)
{
    try
    {
        // 读线程私有模块：目录区解密用（工人另建各自的，互不共享）
        auto modules = Y_flib::StrategyFactory::createModules(mode, password);

        std::vector<std::string> blank;
        BinaryStandardLoader headerLoaderIterator(EncodingUtils::pathToUtf8(fullPath), blank, parentPath);
        headerLoaderIterator.headerLoaderIterator(*modules.encryption);

        // 数据区专用只读流：与 loader 的目录流互不干扰（loader 的 restartLoader 会
        // 整体换流，不能共用）；从数据区起点纯顺序读、零 seek，文件边界靠记账切分
        std::ifstream dataAreaFile(Y_flib::FileSystemUtils::pathForIo(fullPath), std::ios::binary);
        if (!dataAreaFile)
            throw std::runtime_error("decompressionLoop()-Error:Failed to open archive data area: " +
                                     EncodingUtils::pathToUtf8(fullPath));
        Locator locator;
        locator.locateFromBegin(dataAreaFile, headerLoaderIterator.getDirectoryOffset());
        StandardsReader standardsReader(dataAreaFile);

        // 进度回调节流（读线程私有状态）
        auto lastCallbackTime = std::chrono::steady_clock::now();
        double lastReportedProgress = -1.0;

        totalFiles = headerLoaderIterator.fileQueue.size();

        std::uint64_t nextSequence = 1; // 全局序号：块与 FileStart 都占号，保证密集

        while (!headerLoaderIterator.allLoopIsDone())
        {
            // 目录创建在读线程：先于写线程落文件（写侧 createFile 亦会自愈补建父目录）
            while (!headerLoaderIterator.directoryQueueReady.empty())
            {
                std::filesystem::path dirToCreate = headerLoaderIterator.directoryQueueReady.front();
                if (!dirToCreate.is_absolute())
                    dirToCreate = parentPath / dirToCreate;
                createDirectory(dirToCreate);
                headerLoaderIterator.directoryQueueReady.pop();
            }

            while (!headerLoaderIterator.fileQueue.empty())
            {
                const FileTask fileTask = headerLoaderIterator.fileQueue.front();
                const std::filesystem::path outputPath = parentPath / fileTask.entry.getFullPath();
                const Y_flib::FileSize originalSize = fileTask.entry.getFileSizeInDetails();
                Y_flib::FileSize fileCompressedSize = fileTask.compressedSize;
                const std::filesystem::path filename = outputPath.filename();

                // FileStart 占号：先于本文件全部块，写线程据此结算上一文件、开新输出文件
                DecompressionMessage fileStart;
                fileStart.sequence = nextSequence++;
                fileStart.kind = DecompressionMessage::Kind::FileStart;
                fileStart.outputPath = outputPath;
                fileStart.originalSize = originalSize;
                resultQueue.push(std::move(fileStart));

                // 已产字节数按块上限累计（与串行按实际累计等价：每块实际输出 == 上限）
                Y_flib::FileSize totalDecompressedBytes = 0;
                while (totalDecompressedBytes < originalSize && fileCompressedSize > 0)
                {
                    DecompressTask task;
                    task.sequence = nextSequence++;
                    task.expectedOriginal = std::min<Y_flib::FileSize>(
                        Y_flib::Constants::BUFFER_SIZE, originalSize - totalDecompressedBytes);

                    // 元数据块：flag + 长度 + 密文（小对象，不进池）
                    if (!(standardsReader.readBinaryStandards<Y_flib::FlagType>() == Y_flib::FlagType::Separated))
                        throw std::runtime_error("decompressionLoop()-Error:Can't read SEPARATED_FLAG before metadata block");
                    const Y_flib::BlockLength metadataBlockSize =
                        standardsReader.readBinaryStandards<Y_flib::BlockLength>();
                    task.encryptedMetadata.clear();
                    task.encryptedMetadata.resize(metadataBlockSize);
                    StandardsReader::readDataBlock(metadataBlockSize, dataAreaFile, task.encryptedMetadata);
                    fileCompressedSize -= metadataBlockSize;

                    // 数据块：flag + 长度 + 密文（池块）
                    if (!(standardsReader.readBinaryStandards<Y_flib::FlagType>() == Y_flib::FlagType::Separated))
                        throw std::runtime_error("decompressionLoop()-Error:Can't read SEPARATED_FLAG before data block");
                    const Y_flib::BlockLength blockSize =
                        standardsReader.readBinaryStandards<Y_flib::BlockLength>();
                    task.encryptedData = bufferPool.acquire(); // 背压点：借不到就等
                    task.encryptedData.resize(blockSize);
                    StandardsReader::readDataBlock(blockSize, dataAreaFile, task.encryptedData);
                    if (static_cast<Y_flib::FileSize>(dataAreaFile.gcount()) != blockSize)
                        throw std::runtime_error(
                            "decompressionLoop()-Error: Failed to read complete data block. Expected " +
                            std::to_string(blockSize) + " bytes, got " +
                            std::to_string(dataAreaFile.gcount()));
                    fileCompressedSize -= blockSize;

                    totalDecompressedBytes += task.expectedOriginal;

                    taskQueue.push(std::move(task));

                    // 进度回调（GUI 的取消以异常从回调里抛出）
                    reportProgress(filename, totalDecompressedBytes, originalSize,
                                   lastCallbackTime, lastReportedProgress);
                }

                headerLoaderIterator.fileQueue.pop();
                processedFiles++;
            }

            while (headerLoaderIterator.fileQueue.empty() && !headerLoaderIterator.allLoopIsDone())
            {
                headerLoaderIterator.restartLoader();
                headerLoaderIterator.headerLoaderIterator(*modules.encryption);
                totalFiles += headerLoaderIterator.fileQueue.size();
            }
        }

        // 尾批目录（目录区末块解析后的剩余就绪队列）
        while (!headerLoaderIterator.directoryQueueReady.empty())
        {
            std::filesystem::path dirToCreate = headerLoaderIterator.directoryQueueReady.front();
            if (!dirToCreate.is_absolute())
                dirToCreate = parentPath / dirToCreate;
            createDirectory(dirToCreate);
            headerLoaderIterator.directoryQueueReady.pop();
        }

        linkTasksOut = std::move(headerLoaderIterator.linkQueueReady); // 移交（loader 随线程销毁）
    }
    catch (...)
    {
        exceptionOut = std::current_exception(); // 用户取消（回调抛出）与读失败统一走此出口
    }
}

void DecompressionLoop::workerLoop(
    SafeQueue<DecompressTask> &taskQueue,
    SafeQueue<DecompressionMessage> &resultQueue,
    Y_flib::CompressionMode mode,
    const std::string &password)
{
    // 每工人一份私有模块（AES 密钥扩展表、哈夫曼树均为可变成员状态，不可共享）
    auto modules = Y_flib::StrategyFactory::createModules(mode, password);

    // 工人私有中间缓冲，跨任务复用。工人全程不触碰缓冲池（与压缩侧同构）：
    // 池块随任务流经本线程——进时装密文，解密与解压在私有缓冲完成后，以解压
    // 输出原地覆写池块，继续前行。读线程唯一借出、写线程唯一归还，依赖图无环。
    Y_flib::DataBlock decryptedMetadata;
    Y_flib::DataBlock decryptedData;
    decryptedMetadata.reserve(Y_flib::Constants::BUFFER_SIZE);
    decryptedData.reserve(Y_flib::Constants::BUFFER_SIZE);

    DecompressTask task;
    while (taskQueue.waitPop(task)) // 排空契约：setDone 后已入队任务照常处理
    {
        DecompressionMessage result; // kind 默认 Block
        result.sequence = task.sequence;
        try
        {
            modules.encryption->decrypt(task.encryptedMetadata, decryptedMetadata);
            modules.encryption->decrypt(task.encryptedData, decryptedData);
            // 输出块先清长度再解压（容量保留）：池块此刻还装着密文，
            // decompress 的输出语义不清空目标缓冲，残留会污染明文
            task.encryptedData.clear();
            modules.compression->decompress(decryptedMetadata, decryptedData,
                                            task.encryptedData, task.expectedOriginal);
            result.decompressedData = std::move(task.encryptedData); // 所有权随消息移交写线程
        }
        catch (...)
        {
            // 异常随消息走、序号不跳（WriteSorter 密集性前提）。块无论何状态都
            // 照常移交写线程——写线程不写它，只负责归还，唯一归还方的记账不被破坏
            result.exception = std::current_exception();
            result.decompressedData = std::move(task.encryptedData);
        }
        resultQueue.push(std::move(result));
    }
}

void DecompressionLoop::writerLoop(
    SafeQueue<DecompressionMessage> &resultQueue,
    BufferPool &bufferPool,
    std::exception_ptr &exceptionOut)
{
    std::exception_ptr taskException = nullptr;
    try
    {
        WriteSorter<DecompressionMessage> sorter;
        std::unique_ptr<DataExporter> currentExporter; // 按文件开关：reset 即结算上一文件

        // 处理一条已按序取出的消息；返回 false = 命中任务异常，立即停写
        auto handleMessage = [&](DecompressionMessage &message) -> bool
        {
            if (message.kind == DecompressionMessage::Kind::FileStart)
            {
                currentExporter.reset(); // 上一文件收尾（fstream 随之关闭；空文件只有 FileStart）
                createFile(message.outputPath);
                currentExporter = std::make_unique<DataExporter>(message.outputPath);
                return true;
            }
            if (message.exception)
            {
                taskException = message.exception; // 首个任务异常按序重抛
                return false;
            }
            currentExporter->exportDecompressedData(message.decompressedData); // 纯追加
            bufferPool.release(std::move(message.decompressedData));           // 输出块唯一归还方
            return true;
        };

        // 滞留块归还（清理路径用）：只还真正持有池块的（空块/已归还的容量为 0）
        auto returnLeftoverBlock = [&bufferPool](DecompressionMessage &leftover)
        {
            if (leftover.kind == DecompressionMessage::Kind::Block && leftover.decompressedData.capacity() > 0)
                bufferPool.release(std::move(leftover.decompressedData));
        };

        // 序号无洞由结构保证，每条消息入序后连续段总能凑齐刷出
        bool aborted = false;
        std::vector<DecompressionMessage> batch;
        DecompressionMessage message;
        while (!aborted && resultQueue.waitPop(message))
        {
            if (sorter.canWrite(message.sequence, std::move(message)))
            {
                batch = sorter.takeInOrder(); // 刷出编排由 sorter 自理：整段升序交出
                for (DecompressionMessage &ordered : batch)
                {
                    if (!handleMessage(ordered))
                    {
                        aborted = true; // 停写：输出即将废弃，后续块不再写出
                        break;
                    }
                }
            }
        }
        // 循环正常结束时 map 必已排空；map 非空只可能是停在首个任务异常——
        // 未写出的消息连同中断批的余部一起退场还池
        if (aborted)
        {
            for (DecompressionMessage &leftover : sorter.drainAll())
                returnLeftoverBlock(leftover);
            for (DecompressionMessage &leftover : batch)
                returnLeftoverBlock(leftover);
            exceptionOut = taskException;
        }
    }
    catch (...)
    {
        exceptionOut = std::current_exception(); // 写盘 IO 异常同样走统一出口
    }
}

void DecompressionLoop::processLinks(std::queue<LinkTask> &linkTasks)
{
    while (!linkTasks.empty())
    {
        const LinkTask task = linkTasks.front();
        linkTasks.pop();

        if (task.targetPath.empty())
        {
            throw std::runtime_error("Archived Windows link target is empty");
        }

        std::filesystem::path linkPath = task.linkPath;
        if (!linkPath.is_absolute())
        {
            linkPath = parentPath / linkPath;
        }

        // 目标字符串按归档记录原样交给 Windows；外部或缺失目标均允许。
        Y_flib::FileSystemUtils::createLink(
            linkPath, task.targetPath, task.linkType);
    }
}

void DecompressionLoop::reportProgress(
    const std::filesystem::path &filename,
    Y_flib::FileSize totalDecompressedBytes,
    Y_flib::FileSize originalSize,
    std::chrono::steady_clock::time_point &lastCallbackTime,
    double &lastReportedProgress)
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCallbackTime).count();
    double fileProgress = (originalSize > 0) ? (100.0 * totalDecompressedBytes / originalSize) : 100.0;
    double overallProgress = totalFiles > 0 ? (100.0 * (processedFiles + fileProgress / 100.0) / totalFiles) : fileProgress;

    bool shouldReport = (elapsed >= PROGRESS_CALLBACK_INTERVAL_MS) ||
                        (overallProgress - lastReportedProgress >= 5.0) ||
                        (totalDecompressedBytes >= originalSize);

    if (progressCallback && shouldReport)
    {
        progressCallback(EncodingUtils::pathToUtf8(filename), fileProgress, overallProgress, "Decompressing");
        lastCallbackTime = now;
        lastReportedProgress = overallProgress;
    }
}

void DecompressionLoop::createDirectory(const std::filesystem::path &directoryPath)
{
    try
    {
        // 使用兼容层递归创建，确保解压目标超过 MAX_PATH 时仍可落盘。
        Y_flib::FileSystemUtils::createDirectories(directoryPath);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("createDirectory()-Error: " + EncodingUtils::pathToUtf8(directoryPath));
    }
}

bool DecompressionLoop::createFile(const std::filesystem::path &filePath)
{
    try
    {
        if (Y_flib::FileSystemUtils::exists(filePath))
        {
            std::cerr << "fileIsExist: " << filePath << " ,skipped to next \n";
            return false;
        }

        if (!filePath.parent_path().empty() &&
            !Y_flib::FileSystemUtils::exists(filePath.parent_path()))
        {
            createDirectory(filePath.parent_path());
        }

        // 保留归档中的普通路径，仅在创建文件时转换为扩展长度路径。
        std::ofstream outfile(Y_flib::FileSystemUtils::pathForIo(filePath));
        if (!outfile)
        {
            throw std::runtime_error(
                "Failed to create file: " + EncodingUtils::pathToUtf8(filePath));
        }
        outfile.close();
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(
            "createFile()-Error: " + EncodingUtils::pathToUtf8(filePath) +
            " - " + e.what());
    }
    return true;
}
