#pragma once

// ---- 压缩库（Y_flib）核心头：归档文件头写入、策略工厂、通用工具类 ----
#include "../CompressorFileSystem/ArchiveFormat/include/HeaderWriter.h"
#include "../CompressorFileSystem/Strategy/include/StrategyFactory.h"
#include "../CompressorFileSystem/Commons/include/ToolClasses.h"
// ---- .sy 文件图标关联（Windows） ----
#include "../IconHandler.h"
// ---- 压缩/解压主循环（CompressionLoop / DecompressionLoop） ----
#include "../Y_Manager/MainLoop.h"

#include <QObject>
#include <QString>
#include <QStringList>

#include <atomic>
#include <chrono>

/**
 * @brief 压缩/解压工作类（Qt Worker 模式）
 *
 * 在工作线程中执行压缩与解压任务，通过信号向界面线程汇报进度与结果。
 * 典型用法：界面线程先调用 setCompressionParams()/setDecompressionParams()
 * 填参，将本对象 moveToThread() 到工作线程后，经信号触发 doCompression()/
 * doDecompression() 槽函数；任务执行期间可随时调用 requestStop() 请求中止。
 */
class CompressionWorker : public QObject
{
    Q_OBJECT

public:
    /*
     * 调用须知:
     * 1. 这里持有的 QString 路径只在 Qt 界面层暂存。
     * 2. 一旦进入文件系统访问、压缩主循环或策略模块，统一先通过 EncodingUtils
     *    转成 UTF-8 / std::filesystem::path，避免再混用 toStdString() 或
     *    QString::fromStdString() 处理路径。
     */
    explicit CompressionWorker(QObject *parent = nullptr);
    ~CompressionWorker();

    /**
     * @brief 设置压缩任务参数（须在触发 doCompression() 之前调用）
     * @param files     待压缩文件路径列表
     * @param outputDir 输出目录
     * @param fileName  压缩包文件名（带不带 .sy 后缀均可，主流程内部会统一剥离）
     * @param password  加密口令
     * @param mode      压缩模式（算法及是否加密），默认 Huffman + AES
     */
    void setCompressionParams(const QStringList &files,
                              const QString &outputDir,
                              const QString &fileName,
                              const QString &password,
                              Y_flib::CompressionMode mode = Y_flib::CompressionMode::HuffmanAES);

    /**
     * @brief 设置解压任务参数（须在触发 doDecompression() 之前调用）
     * @param inputFile 输入的 .sy 压缩包路径
     * @param outputDir 解压输出目录
     * @param password  解密口令；未加密的包可留空
     */
    void setDecompressionParams(const QString &inputFile,
                                const QString &outputDir,
                                const QString &password);

    /** @brief 请求中止当前任务。线程安全：仅置位原子标记，实际中断由任务内部各检查点协作完成。 */
    void requestStop() { m_stopRequested.store(true); }

    /** @brief 查询是否已请求中止 */
    bool isStopRequested() const { return m_stopRequested.load(); }

    /**
     * @brief 复位中止标记，并重置进度节流的基准值
     * @note 每次任务开始（进入 doCompression()/doDecompression()）时调用，
     *       避免上一次任务遗留的取消状态与节流记录串扰
     */
    void resetStopFlag()
    {
        m_stopRequested.store(false);
        m_lastProgressTime = std::chrono::steady_clock::now();
        m_lastEmittedProgress = -1.0;
    }

public slots:
    /** @brief 压缩任务入口：校验参数 -> 写归档头 -> 压缩主循环 -> 关联图标 */
    void doCompression();

    /** @brief 解压任务入口：校验参数 -> 探测归档头/加密需求 -> 解压主循环 */
    void doDecompression();

signals:
    /**
     * @brief 进度汇报信号（内部已按时间/幅度节流）
     * @param filename        当前正在处理的文件名
     * @param fileProgress    单文件进度（0~100）
     * @param overallProgress 整体进度（0~100，主循环原始进度已线性映射到任务阶段刻度）
     * @param status          当前阶段/状态描述
     */
    void detailedProgress(const QString &filename, double fileProgress, double overallProgress, const QString &status);

    /**
     * @brief 任务结束信号。无论成功、失败还是用户取消，每次任务恰好发出一次
     * @param success 任务是否成功
     * @param message 结果描述（成功为输出位置；取消/失败为原因）
     */
    void finished(bool success, const QString &message);

private:
    /** @brief 校验压缩参数（待压缩文件存在、输出目录有效）；失败时发出 finished(false, 原因) 并返回 false */
    bool validateCompressionParams();

    /** @brief 校验解压参数（压缩包存在、后缀为 .sy）；失败时发出 finished(false, 原因) 并返回 false */
    bool validateDecompressionParams();

    /**
     * @brief 进度信号节流判断
     * @return 满足任一条件返回 true：距上次发送 >= PROGRESS_INTERVAL_MS、
     *         进度增幅 >= PROGRESS_DELTA、进度已达 100%（返回 true 时同步更新节流基准）
     */
    bool shouldEmitProgress(double currentProgress);

    // ---------- 压缩任务参数（QString 仅在界面层暂存，进入库层前统一转 UTF-8） ----------
    QStringList m_filesToCompress;              ///< 待压缩文件列表
    QString m_outputDir;                        ///< 输出目录
    QString m_outputFileName;                   ///< 压缩包文件名（不含目录部分）
    QString m_password;                         ///< 加密口令
    Y_flib::CompressionMode m_mode{Y_flib::CompressionMode::HuffmanAES}; ///< 压缩模式

    // ---------- 解压任务参数 ----------
    QString m_decompressInputFile;              ///< 输入压缩包路径
    QString m_decompressOutputDir;              ///< 解压输出目录
    QString m_decompressPassword;               ///< 解密口令

    // ---------- 取消与进度节流 ----------
    std::atomic<bool> m_stopRequested{false};   ///< 协作式取消标记（界面线程置位，工作线程检查）
    std::chrono::steady_clock::time_point m_lastProgressTime; ///< 上次发出进度信号的时刻
    double m_lastEmittedProgress{-1.0};         ///< 上次发出的整体进度（-1 表示尚未发过）

    // 节流阈值：距上次发送 >= 200ms 或进度跳变 >= 2% 才发信号，
    // 兼顾状态文本的刷新频率与界面线程事件队列的负载
    static constexpr int PROGRESS_INTERVAL_MS = 200;
    static constexpr double PROGRESS_DELTA = 2.0;
};
