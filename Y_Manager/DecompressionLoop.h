#pragma once

#include "../CompressorFileSystem/Commons/include/FileLibrary.h"
#include "../CompressorFileSystem/DataIO/include/DataExporter.h"
#include "../CompressorFileSystem/ArchiveFormat/include/BinaryStandardLoader.h"
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
#include <queue>
#include <thread>
#include <vector>

// 进度回调函数类型: (当前文件名, 当前文件进度百分比, 整体进度百分比, 状态消息)
using ProgressCallback = std::function<void(const std::string &, double, double, const std::string &)>;

/* DecompressionLoop - 解压流水线总指挥（与 CompressionLoop 同构，组件复用）
 *
 * 三段流水线：
 *   专职读线程 ──任务队列──> N 个计算工人 ──结果队列──> 专职写线程
 *         │                                                  │
 *         └────────── BufferPool（在途大块上限，唯一背压点）──────┘
 *
 *   读线程：独占 BinaryStandardLoader（目录解析/目录区解密）与数据区专用 ifstream
 *           （从 directoryOffset 纯顺序读、零 seek，文件边界按 fileQueue 顺序 +
 *           compressedSize 记账，同串行逻辑）；目录创建在此线程（先于写线程落文件）；
 *           每文件先发 FileStart，再逐块对发任务；进度回调与取消检查只在此线程；
 *           结束移交链接任务队列。
 *   工人  ：开头各自 createModules(mode, password) 一份私有模块；两次解密落私有缓冲，
 *           解压输出原地覆写池块（密文→明文）；零池调用；异常随消息走、序号不跳。
 *   写线程：WriteSorter 按序号重排；FileStart → 结算上一文件 + 建新文件与 DataExporter；
 *           Block → 纯追加写盘 + 还池；首个任务异常按序重抛停刷。
 *
 * 关闭协议（与压缩侧同构，总指挥集中关）：
 *   join 读 → 关任务队列 → join 工人 → 关结果队列 → join 写 → processLinks（链接
 *   严格最后，目标可能是已还原文件）→ 关池。错误/取消统一重抛首异常。
 *
 * 输出字节等价：写出顺序 = 序号顺序 = 归档物理顺序 = 串行写出顺序；每块解压输出
 * 上限与串行 running-remaining 数学等价（见 DecompressionMessages.h）。
 */
class DecompressionLoop
{
private:
    std::filesystem::path parentPath;
    std::filesystem::path fullPath;
    ProgressCallback progressCallback;
    Y_flib::FileSize totalFiles;     // 仅读线程访问（进度口径）
    Y_flib::FileSize processedFiles; // 仅读线程访问

    /* 专职读线程：目录装载 + 数据区顺序读块 + FileStart/任务发送 + 进度与取消 */
    void readerLoop(
        SafeQueue<DecompressTask> &taskQueue,
        SafeQueue<DecompressionMessage> &resultQueue,
        Y_flib::BufferPool &bufferPool,
        Y_flib::CompressionMode mode,
        const std::string &password,
        std::queue<Y_flib::LinkTask> &linkTasksOut,
        std::exception_ptr &exceptionOut);

    /* 计算工人：两次解密 + 解压；池块原地变换（明文覆写密文）；异常随消息走 */
    void workerLoop(
        SafeQueue<DecompressTask> &taskQueue,
        SafeQueue<DecompressionMessage> &resultQueue,
        Y_flib::CompressionMode mode,
        const std::string &password);

    /* 专职写线程：定序 + 按文件开关 DataExporter + 纯追加写盘 + 还池；异常统一出口 */
    void writerLoop(
        SafeQueue<DecompressionMessage> &resultQueue,
        Y_flib::BufferPool &bufferPool,
        std::exception_ptr &exceptionOut);

    /* 流水线排空后由总指挥串行执行：重建 Windows 链接（目标可能是已还原的文件/目录） */
    void processLinks(std::queue<Y_flib::LinkTask> &linkTasks);

    /* 报告进度（读线程私有调用） */
    void reportProgress(const std::filesystem::path &filename,
                        Y_flib::FileSize totalDecompressedBytes,
                        Y_flib::FileSize originalSize,
                        std::chrono::steady_clock::time_point &lastCallbackTime,
                        double &lastReportedProgress);

    /* 创建目录（兼容层递归创建，超 MAX_PATH 可落盘） */
    void createDirectory(const std::filesystem::path &directoryPath);

    /* 创建输出文件（存在则覆盖，缺父目录则先建） */
    bool createFile(const std::filesystem::path &filePath);

public:
    DecompressionLoop(std::string deCompressionFilePath, std::string outputDirectory = "")
        : progressCallback(nullptr), totalFiles(0), processedFiles(0)
    {
        fullPath = Y_flib::EncodingUtils::pathFromUtf8(deCompressionFilePath);

        if (outputDirectory.empty() || outputDirectory == ".")
        {
            parentPath = fullPath.parent_path();
        }
        else
        {
            parentPath = Y_flib::EncodingUtils::pathFromUtf8(outputDirectory);
        }
    }

    void setProgressCallback(ProgressCallback callback)
    {
        progressCallback = callback;
    }

    /* 模块组装下沉进流水线：读线程与工人各自按 (mode, password) 制造私有模块
     *（mode 由调用方从归档头探测：StrategyFactory::idToMode(header.strategy)） */
    void decompressionLoop(Y_flib::CompressionMode mode,
                           const std::string &password);
};
