#pragma once

#include "../CompressorFileSystem/Commons/include/FileLibrary.h"
#include "../CompressorFileSystem/DataIO/include/DataLoader.h"
#include "../CompressorFileSystem/DataIO/include/DataExporter.h"
#include "../CompressorFileSystem/ArchiveFormat/include/BinaryStandardLoader.h"
#include "../CompressorFileSystem/ArchiveFormat/include/CatalogFinalizer.h"
#include "../CompressorFileSystem/Commons/include/ToolClasses.h"
#include "../BufferPool/BufferPool.h"
#include "../ThreadPool/SafeQueue.h"
#include "../ThreadPool/WriteSorter.h"
#include "PipelineMessages.h"
#include <filesystem>
#include <functional>
#include <string>
#include <chrono>
#include <exception>
#include <thread>
#include <vector>

// 进度回调函数类型: (当前文件名, 当前文件进度百分比, 整体进度百分比, 状态消息)
using ProgressCallback = std::function<void(const std::string &, double, double, const std::string &)>;

/* CompressionLoop - 压缩流水线总指挥
 *
 * 三段流水线（固定角色，取材 ThreadLab 验证过的模型）：
 *   专职读线程 ──任务队列──> N 个计算工人 ──结果队列──> 专职写线程
 *         │                                                  │
 *         └──────────── BufferPool（在途大块上限，唯一背压点）────────┘
 *
 *   读线程：目录装载 + 逐块借池块、复制数据、分配全局递增序号、提交任务；
 *           进度回调与取消检查只在此线程（GUI 的取消以异常从回调抛出）；
 *           文件完成发结算消息（空文件只有它），结束移交目录块位置表。
 *   工人  ：开头各自 createModules(mode, password) 一份私有模块（AES 轮密钥、
 *           哈夫曼树均为可变成员状态，不可共享）；压缩 + 两次加密，池块原地
 *           变换后随结果前行，不碰池；异常随消息存储。
 *   写线程：WriteSorter 按序号重排（区段连续性判据），DataExporter 纯追加写盘，
 *           攒 SizeFillEntry 表；首个任务异常按序重抛。
 *
 * 关闭协议（总指挥集中关，顺序反了会丢尾块或永久等待）：
 *   join 读 → 关任务队列 → join 工人 → 关结果队列 → join 写 → CatalogFinalizer → 关池。
 *   错误路径：任一专职线程异常先行退出，总指挥仍按上序收口后统一重抛首异常；
 *   写线程在首个任务异常处停刷，滞留池块逐条归还后退出。
 *   序号无洞是结构保证：读端单线程按序分配、工人每任务必产出、排空契约不丢。
 */
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
    void prepareNextFile(
        Y_flib::DataLoader *dataLoader,
        Y_flib::EntryDetails &fileEntry,
        std::filesystem::path &filename,
        Y_flib::FileSize &totalBlocks,
        Y_flib::FileSize &blockCount);

    /* 专职读线程：目录装载 + 借块/复制/编号/提交 + 文件结算 + 进度与取消 */
    void readerLoop(
        SafeQueue<BlockTask> &taskQueue,
        SafeQueue<PipelineMessage> &resultQueue,
        Y_flib::BufferPool &bufferPool,
        const std::vector<std::string> &filePathToScan,
        Y_flib::CompressionMode mode,
        const std::string &password,
        std::vector<Y_flib::BlockSpan> &blockSpansOut,
        std::exception_ptr &exceptionOut);

    /* 计算工人：压缩 + 两次加密。全程不触碰缓冲池——池块随任务流经本线程，
     * 加密输出原地覆写输入后随结果继续前行（ThreadLab 同构：读线程唯一借出、
     * 写线程唯一归还，依赖图无环，结构性无死锁）；异常随消息走、序号不跳 */
    void workerLoop(
        SafeQueue<BlockTask> &taskQueue,
        SafeQueue<PipelineMessage> &resultQueue,
        Y_flib::CompressionMode mode,
        const std::string &password);

    /* 专职写线程：定序 + 纯追加写盘 + 还池 + 攒结算表；异常统一出口 */
    void writerLoop(
        SafeQueue<PipelineMessage> &resultQueue,
        Y_flib::BufferPool &bufferPool,
        std::vector<Y_flib::SizeFillEntry> &sizeFillEntriesOut,
        std::exception_ptr &exceptionOut);

public:
    CompressionLoop(const std::string compressionFilePath)
        : compressionFilePath(compressionFilePath), progressCallback(nullptr), totalFiles(0), processedFiles(0)
    {
    }

    void setProgressCallback(ProgressCallback callback)
    {
        progressCallback = callback;
    }

    /* 模块组装下沉进流水线：工人各自按 (mode, password) 制造私有模块，
     * 不再从调用方传入共享模块引用 */
    void compressionLoop(const std::vector<std::string> &filePathToScan,
                         Y_flib::CompressionMode mode,
                         const std::string &password);
};
