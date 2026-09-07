#include "CompressionLoop.h"
#include "../CompressorFileSystem/Commons/include/FileSystemUtils.h"
#include "../CompressorFileSystem/Strategy/include/StrategyFactory.h"
#include <chrono>
#include <memory>
#include <utility>

using Y_flib::BinaryStandardLoader;
using Y_flib::BufferPool;
using Y_flib::DataExporter;
using Y_flib::DataLoader;
using Y_flib::EncodingUtils;
using Y_flib::EntryDetails;

// 进度回调最小间隔（毫秒）
static constexpr int PROGRESS_CALLBACK_INTERVAL_MS = 100;

// 计算工人数量：物理并行度，取不到时退回 4
static unsigned pipelineWorkerCount()
{
    const unsigned count = std::thread::hardware_concurrency();
    return count == 0 ? 4u : count;
}

void CompressionLoop::compressionLoop(
    const std::vector<std::string> &filePathToScan,
    Y_flib::CompressionMode mode,
    const std::string &password)
{
    const unsigned workerCount = pipelineWorkerCount();
    // 池容量 = 在途块上限 = 唯一背压点（读线程借不到就等，下游的慢拖住上游）。
    // 块大小留 1KB 余量：不可压缩的 8MB 块加密后是 8MB+IV，原地覆写会溢出恰好
    // 8MB 的池块（串行版 encryptedBlock 预留双倍是同一考虑）。
    // 工人全程不碰池，容量只需覆盖「排队任务 + 处理中 + 待按序刷出的结果」，
    // 2×(N+2) 为这一在途量留了余量
    BufferPool bufferPool(2u * (workerCount + 2u),
                          static_cast<std::size_t>(Y_flib::Constants::BUFFER_SIZE) + 1024);

    SafeQueue<BlockTask> taskQueue;
    SafeQueue<PipelineMessage> resultQueue;

    // 各线程的产出与异常出口，join 之后由总指挥取用
    std::vector<Y_flib::SizeFillEntry> sizeFillEntries; // 写线程攒的结算表
    std::vector<Y_flib::BlockSpan> directoryBlockSpans; // 读线程移交的目录块位置表
    std::exception_ptr readerException = nullptr;
    std::exception_ptr writerException = nullptr;

    // 计算总文件数用于进度报告
    countTotalFiles(filePathToScan);

    // 启动顺序：写 → 工人 → 读（读线程最先结束，逐级收口）
    std::thread writerThread(&CompressionLoop::writerLoop, this,
                             std::ref(resultQueue), std::ref(bufferPool),
                             std::ref(sizeFillEntries), std::ref(writerException));
    std::vector<std::thread> workerThreads;
    workerThreads.reserve(workerCount);
    for (unsigned workerIndex = 0; workerIndex < workerCount; ++workerIndex)
        workerThreads.emplace_back(&CompressionLoop::workerLoop, this,
                                   std::ref(taskQueue), std::ref(resultQueue),
                                   mode, password);
    std::thread readerThread(&CompressionLoop::readerLoop, this,
                             std::ref(taskQueue), std::ref(resultQueue),
                             std::ref(bufferPool), std::cref(filePathToScan),
                             mode, password,
                             std::ref(directoryBlockSpans), std::ref(readerException));

    // 集中收口：读自然结束（或带异常退出）→ 关任务队列 → 工人排空退出
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

    // 收尾：目录区唯一改动者，类内焊死「先回填大小槽，后原地加密」
    auto modules = Y_flib::StrategyFactory::createModules(mode, password);
    Y_flib::CatalogFinalizer catalogFinalizer(EncodingUtils::pathFromUtf8(compressionFilePath));
    catalogFinalizer.finalize(sizeFillEntries, directoryBlockSpans, *modules.encryption, mode);

    bufferPool.close();

    // 完成回调
    if (progressCallback)
    {
        progressCallback("", 100.0, 100.0, "Completed");
    }
}

void CompressionLoop::readerLoop(
    SafeQueue<BlockTask> &taskQueue,
    SafeQueue<PipelineMessage> &resultQueue,
    BufferPool &bufferPool,
    const std::vector<std::string> &filePathToScan,
    Y_flib::CompressionMode mode,
    const std::string &password,
    std::vector<Y_flib::BlockSpan> &blockSpansOut,
    std::exception_ptr &exceptionOut)
{
    try
    {
        auto modules = Y_flib::StrategyFactory::createModules(mode, password);

        // 初始化迭代器
        std::filesystem::path blank;
        BinaryStandardLoader headerLoaderIterator(compressionFilePath, filePathToScan, blank);

        std::filesystem::path loadPath;
        std::unique_ptr<DataLoader> dataLoader;
        Y_flib::FileSize totalBlocks = 1, blockCount = 0;
        std::uint64_t nextSequence = 1; // 全局序号：块与文件结算都占号，保证密集

        // 进度回调节流（读线程私有状态）
        auto lastCallbackTime = std::chrono::steady_clock::now();
        double lastReportedProgress = -1.0;

        headerLoaderIterator.headerLoaderIterator(*modules.encryption); // 执行第一次操作，把根目录载入
        // 首个目录块可能只有目录条目而没有文件条目（BFS 层序 + 16KB 分割所致），
        // 需持续拉块直到出现文件条目或目录读取完成，否则主循环会因队列空而静默跳过
        while (headerLoaderIterator.fileQueue.empty() && !headerLoaderIterator.allLoopIsDone())
        {
            headerLoaderIterator.restartLoader();
            headerLoaderIterator.headerLoaderIterator(*modules.encryption);
        }
        if (!headerLoaderIterator.fileQueue.empty()) // 单个文件特殊处理
        {
            EntryDetails loadFile = headerLoaderIterator.fileQueue.front().entry;
            loadPath = loadFile.getFullPath();
            dataLoader = std::make_unique<DataLoader>(loadPath);
            totalBlocks = (loadFile.getFileSizeInDetails() + Y_flib::Constants::BUFFER_SIZE - 1) / Y_flib::Constants::BUFFER_SIZE;
        }

        std::filesystem::path filename = loadPath.filename();
        while (!headerLoaderIterator.fileQueue.empty())
        {
            dataLoader->dataLoader();

            if (!dataLoader->isDone() && blockCount < totalBlocks) // 处理当前文件的每个数据块
            {
                blockCount++;

                BlockTask task;
                task.sequence = nextSequence++;
                task.inputData = bufferPool.acquire();   // 背压点：借不到块就等，下游的慢拖住上游
                task.inputData = dataLoader->getBlock(); // 装载器内部缓冲会被覆写，必须复制进池块

                taskQueue.push(std::move(task));

                // 计算进度并回调（GUI 的取消以异常从回调里抛出）
                reportProgress(filename, blockCount, totalBlocks, lastCallbackTime, lastReportedProgress);
            }

            if (dataLoader->isDone() && !headerLoaderIterator.fileQueue.empty()) // 当前文件完成 → 结算
            {
                PipelineMessage settlement;
                settlement.sequence = nextSequence++; // 紧跟本文件最后一块、先于下一文件首块
                settlement.kind = PipelineMessage::Kind::Settlement;
                settlement.processedSizeOffset = headerLoaderIterator.fileQueue.front().processedSizeOffset;
                resultQueue.push(std::move(settlement));

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
                headerLoaderIterator.headerLoaderIterator(*modules.encryption);

                if (!headerLoaderIterator.fileQueue.empty())
                {
                    prepareNextFile(dataLoader.get(), headerLoaderIterator.fileQueue.front().entry,
                                    filename, totalBlocks, blockCount);
                }
            }
        }
        blockSpansOut = headerLoaderIterator.takeBlockSpans(); // 移交目录块位置表（收尾加密用）
    }
    catch (...)
    {
        exceptionOut = std::current_exception(); // 用户取消（回调抛出）与读失败统一走此出口
    }
}

void CompressionLoop::workerLoop(
    SafeQueue<BlockTask> &taskQueue,
    SafeQueue<PipelineMessage> &resultQueue,
    Y_flib::CompressionMode mode,
    const std::string &password)
{
    // 每工人一份私有模块（AES 密钥扩展表、哈夫曼频率表/树根都是可变成员状态，不可共享）
    auto modules = Y_flib::StrategyFactory::createModules(mode, password);

    // 工人私有的中间缓冲，跨任务复用。工人全程不触碰缓冲池（ThreadLab 同构）：
    // 池块随消息流经本线程——进时装明文输入，压缩后以加密输出原地覆写，继续前行。
    // 读线程是唯一借出方、写线程是唯一归还方，依赖图无环，结构性无死锁。
    Y_flib::DataBlock metadata;
    Y_flib::DataBlock compressedData;
    metadata.reserve(Y_flib::Constants::BUFFER_SIZE);
    compressedData.reserve(Y_flib::Constants::BUFFER_SIZE);

    BlockTask task;
    while (taskQueue.waitPop(task)) // 排空契约：setDone 后已入队任务照常处理
    {
        PipelineMessage result; // kind 默认 Block
        result.sequence = task.sequence;
        try
        {
            metadata.clear();
            compressedData.clear();
            modules.compression->compress(task.inputData, metadata, compressedData);

            modules.encryption->encrypt(metadata, result.encryptedMetadata); // 小对象随消息走，不进池
            modules.encryption->encrypt(compressedData, task.inputData);     // 原地变换：明文块覆写为密文块
            result.encryptedData = std::move(task.inputData);                // 块的所有权随消息移交写线程
        }
        catch (...)
        {
            // 异常随消息走、序号不跳（WriteSorter 密集性前提）。块无论何状态都照常
            // 移交写线程——写线程不写它，只负责归还，唯一归还方的记账不被破坏
            result.exception = std::current_exception();
            result.encryptedMetadata.clear();
            result.encryptedData = std::move(task.inputData);
        }
        resultQueue.push(std::move(result));
    }
}

void CompressionLoop::writerLoop(
    SafeQueue<PipelineMessage> &resultQueue,
    BufferPool &bufferPool,
    std::vector<Y_flib::SizeFillEntry> &sizeFillEntriesOut,
    std::exception_ptr &exceptionOut)
{
    std::exception_ptr taskException = nullptr;
    try
    {
        DataExporter dataExporter(EncodingUtils::pathFromUtf8(compressionFilePath));
        WriteSorter<PipelineMessage> sorter;

        // 处理一条已按序取出的消息；返回 false = 命中任务异常，立即停写
        auto handleMessage = [&](PipelineMessage &message) -> bool
        {
            if (message.kind == PipelineMessage::Kind::Settlement)
            {
                sizeFillEntriesOut.push_back({message.processedSizeOffset,
                                              dataExporter.currentFileProcessedSize()});
                dataExporter.startNextFile();
                return true;
            }
            if (message.exception)
            {
                taskException = message.exception; // 首个任务异常按序重抛
                return false;
            }
            dataExporter.exportCompressedData(message.encryptedMetadata); // 元数据块在前
            dataExporter.exportCompressedData(message.encryptedData);     // 数据块在后（与串行同布局）
            bufferPool.release(std::move(message.encryptedData));         // 输出块是唯一归还方，无死锁
            return true;
        };

        // 滞留块归还（清理路径用）：只还真正持有池块的（空块/已归还的容量为 0）
        auto returnLeftoverBlock = [&bufferPool](PipelineMessage &leftover)
        {
            if (leftover.kind == PipelineMessage::Kind::Block && leftover.encryptedData.capacity() > 0)
                bufferPool.release(std::move(leftover.encryptedData));
        };

        // 序号无洞由结构保证（读端按序分配、工人每任务必产出、排空契约不丢），
        // 所以每条消息入序后，紧接已写前缀的连续段总能凑齐刷出
        bool aborted = false;
        std::vector<PipelineMessage> batch;
        PipelineMessage message;
        while (!aborted && resultQueue.waitPop(message))
        {
            if (sorter.canWrite(message.sequence, std::move(message)))
            {
                batch = sorter.takeInOrder(); // 刷出编排由 sorter 自理：整段升序交出
                for (PipelineMessage &ordered : batch)
                {
                    if (!handleMessage(ordered))
                    {
                        aborted = true; // 停刷：归档即将废弃，后续块不再写出
                        break;
                    }
                }
            }
        }
        // 循环正常结束时 map 必已排空（末条消息凑齐尾段）；走到这里 map 非空
        // 只可能是停在首个任务异常——未写出的消息连同中断批的余部一起退场还池
        if (aborted)
        {
            for (PipelineMessage &leftover : sorter.drainAll())
                returnLeftoverBlock(leftover);
            for (PipelineMessage &leftover : batch)
                returnLeftoverBlock(leftover);
            exceptionOut = taskException;
        }
    }
    catch (...)
    {
        exceptionOut = std::current_exception(); // 写盘 IO 异常同样走统一出口
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
                        if (info.isReparsePoint)
                        {
                            // 链接只有目录元数据，不读取目标内容，也不计入文件进度。
                            continue;
                        }
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
