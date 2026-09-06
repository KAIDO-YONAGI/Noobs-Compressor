#include "CompressionWorker.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

using Y_flib::EncodingUtils;
using Y_flib::HeaderWriter;

/**
 * @brief 进度信号节流：判断 currentProgress 是否应当发给界面
 *
 * 满足任一条件即放行并刷新节流基准：
 * 1. 距上次发送 >= PROGRESS_INTERVAL_MS —— 进度长时间不动时也能定期刷新状态文本；
 * 2. 进度增幅 >= PROGRESS_DELTA —— 大文件快速推进时按幅度刷新；
 * 3. 进度 >= 100% —— 收尾必发，保证界面能落到 100%。
 */
bool CompressionWorker::shouldEmitProgress(double currentProgress)
{
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastProgressTime).count();

    const bool shouldEmit = (elapsed >= PROGRESS_INTERVAL_MS) ||
                            (currentProgress - m_lastEmittedProgress >= PROGRESS_DELTA) ||
                            (currentProgress >= 100.0);

    if (shouldEmit)
    {
        m_lastProgressTime = now;
        m_lastEmittedProgress = currentProgress;
    }

    return shouldEmit;
}

CompressionWorker::CompressionWorker(QObject *parent)
    : QObject(parent)
{
}

CompressionWorker::~CompressionWorker() = default;

void CompressionWorker::setCompressionParams(const QStringList &files,
                                             const QString &outputDir,
                                             const QString &fileName,
                                             const QString &password,
                                             Y_flib::CompressionMode mode)
{
    m_filesToCompress = files;
    m_outputDir = outputDir;
    m_outputFileName = fileName;
    m_password = password;
    m_mode = mode;
}

void CompressionWorker::setDecompressionParams(const QString &inputFile,
                                               const QString &outputDir,
                                               const QString &password)
{
    m_decompressInputFile = inputFile;
    m_decompressOutputDir = outputDir;
    m_decompressPassword = password;
}

/**
 * @brief 校验压缩参数：逐个确认待压缩文件存在，并确认输出目录有效
 *
 * 任一项不通过都会发出 finished(false, 原因) 并返回 false，调用方据此直接结束任务。
 * 路径统一经 EncodingUtils::qStringToPath 转换；非法路径（含非法字符等）
 * 会以 std::filesystem 异常抛出，这里统一按“路径无效”上报。
 */
bool CompressionWorker::validateCompressionParams()
{
    for (const QString &file : m_filesToCompress)
    {
        try
        {
            if (!std::filesystem::exists(EncodingUtils::qStringToPath(file)))
            {
                emit finished(false, QStringLiteral("File not found: %1").arg(file));
                return false;
            }
        }
        catch (...)
        {
            emit finished(false, QStringLiteral("Invalid path: %1").arg(file));
            return false;
        }
    }

    try
    {
        const std::filesystem::path outputPath = EncodingUtils::qStringToPath(m_outputDir);
        if (!std::filesystem::is_directory(outputPath))
        {
            emit finished(false, QStringLiteral("Output directory not found: %1").arg(m_outputDir));
            return false;
        }
    }
    catch (...)
    {
        emit finished(false, QStringLiteral("Invalid output directory: %1").arg(m_outputDir));
        return false;
    }

    return true;
}

/**
 * @brief 校验解压参数：确认压缩包存在且扩展名为 .sy
 * @return 通过返回 true；否则发出 finished(false, 原因) 并返回 false
 */
bool CompressionWorker::validateDecompressionParams()
{
    try
    {
        if (!std::filesystem::exists(EncodingUtils::qStringToPath(m_decompressInputFile)))
        {
            emit finished(false, QStringLiteral("Archive file not found: %1").arg(m_decompressInputFile));
            return false;
        }
    }
    catch (...)
    {
        emit finished(false, QStringLiteral("Invalid archive path: %1").arg(m_decompressInputFile));
        return false;
    }

    if (!m_decompressInputFile.toLower().endsWith(".sy"))
    {
        emit finished(false, QStringLiteral("Only .sy files can be decompressed"));
        return false;
    }

    return true;
}

/**
 * @brief 压缩任务主流程（槽函数，在工作线程中执行）
 *
 * 流程：复位取消标记 -> 参数校验 -> 整理输出路径（统一 UTF-8）->
 *       按模式创建加密/压缩策略模块 -> 写归档文件头 -> 压缩主循环 ->
 *       尝试关联图标 -> 发出 finished(true)。
 *
 * 取消机制：各阶段间隙检查取消标记；主循环内的取消通过进度回调抛出
 * std::runtime_error("Operation cancelled by user") 使循环展开，外层
 * 按该约定文本识别“用户取消”，与真实失败分开汇报。
 *
 * 进度刻度：主循环把库层的 overallProgress(0~100) 线性映射到 20~95；
 * 校验/建模块/写头等前置阶段占用 0~20，收尾占 95~100。
 */
void CompressionWorker::doCompression()
{
    resetStopFlag();
    emit detailedProgress("", 0.0, 0.0, tr("Validating parameters..."));

    if (!validateCompressionParams())
    {
        return;
    }

    try
    {
        if (isStopRequested())
        {
            emit finished(false, tr("Compression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 5.0, tr("Preparing files..."));

        // 库层接口统一吃 UTF-8 路径，这里做一次批量转换
        std::vector<std::string> filePathToScan;
        filePathToScan.reserve(static_cast<size_t>(m_filesToCompress.size()));
        for (const QString &file : m_filesToCompress)
        {
            filePathToScan.push_back(EncodingUtils::qStringToUtf8(file));
        }

        // 用户填的包名允许带 .sy 后缀，这里统一剥掉，落盘时再补回
        std::string outputFileName = EncodingUtils::qStringToUtf8(m_outputFileName);
        if (outputFileName.size() > 3 && outputFileName.substr(outputFileName.size() - 3) == ".sy")
        {
            outputFileName.erase(outputFileName.size() - 3);
        }

        const std::filesystem::path compressionFilePath =
            EncodingUtils::qStringToPath(m_outputDir) / EncodingUtils::pathFromUtf8(outputFileName + ".sy");
        std::string compressionFilePathUtf8 = EncodingUtils::pathToUtf8(compressionFilePath);
        // 归档内部的逻辑根目录名（去掉后缀的包名），解压时以此为根还原
        const std::string logicalRoot = outputFileName;

        if (isStopRequested())
        {
            emit finished(false, tr("Compression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 10.0, tr("Creating strategy modules..."));

        // 依据模式实例化加密/压缩策略，口令在此交给策略模块持有
        auto modules = Y_flib::StrategyFactory::createModules(m_mode, EncodingUtils::qStringToUtf8(m_password));

        if (isStopRequested())
        {
            emit finished(false, tr("Compression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 15.0, tr("Writing file header..."));

        // 先写归档头：记录文件清单、逻辑根目录与所用策略，解压端据此还原
        HeaderWriter headerWriter;
        headerWriter.headerWriter(filePathToScan, compressionFilePathUtf8, logicalRoot, m_mode);

        if (isStopRequested())
        {
            emit finished(false, tr("Compression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 20.0, tr("Starting compression..."));

        CompressionLoop compressor(compressionFilePathUtf8);
        compressor.setProgressCallback([this](const std::string &filename,
                                              double fileProgress,
                                              double overallProgress,
                                              const std::string &status) {
            if (isStopRequested())
            {
                // 以异常方式从主循环中跳出，是主循环阶段唯一的取消通道
                throw std::runtime_error("Operation cancelled by user");
            }

            // 库层进度 0~100 线性映射到任务刻度 20~95
            const double mappedProgress = 20.0 + overallProgress * 0.75;
            if (shouldEmitProgress(mappedProgress))
            {
                emit detailedProgress(EncodingUtils::utf8ToQString(filename),
                                      fileProgress,
                                      mappedProgress,
                                      EncodingUtils::utf8ToQString(status));
            }
        });
        compressor.compressionLoop(filePathToScan, *modules.encryption, *modules.compression, m_mode);

        if (isStopRequested())
        {
            emit finished(false, tr("Compression cancelled by user"));
            return;
        }

        // 给 .sy 文件关联图标属于增值步骤，失败不影响压缩结果，静默忽略
        try
        {
            IconHandler::AssociateIconToSyFile(compressionFilePathUtf8, "");
        }
        catch (...)
        {
        }

        emit detailedProgress("", 100.0, 100.0, tr("Completed"));
        emit finished(true, QStringLiteral("Compression successful!\nOutput file: %1")
                                 .arg(EncodingUtils::pathToQString(compressionFilePath)));
    }
    catch (const std::exception &e)
    {
        // 按约定文本识别“用户取消”，避免被当作错误上报
        if (std::string(e.what()) == "Operation cancelled by user")
        {
            emit finished(false, QStringLiteral("Compression cancelled by user"));
        }
        else
        {
            emit finished(false, QStringLiteral("Compression failed: %1")
                                     .arg(EncodingUtils::utf8ToQString(e.what())));
        }
    }
    catch (...)
    {
        emit finished(false, QStringLiteral("Compression failed due to unknown error"));
    }
}

/**
 * @brief 解压任务主流程（槽函数，在工作线程中执行）
 *
 * 流程：复位取消标记 -> 参数校验 -> 读归档头并校验魔数 ->
 *       依头部 strategy 字段探测压缩模式 -> 加密包缺口令则提前终止 ->
 *       创建策略模块 -> 解压主循环 -> 发出 finished(true)。
 *
 * 与压缩不同：压缩模式不由外部传入，而是从归档头部探测得到，
 * 因此解压无需用户选择算法；主循环进度映射到 15~95（读头/建模块
 * 等前置阶段占用 0~15），收尾占 95~100。
 */
void CompressionWorker::doDecompression()
{
    resetStopFlag();
    emit detailedProgress("", 0.0, 0.0, tr("Validating parameters..."));

    if (!validateDecompressionParams())
    {
        return;
    }

    try
    {
        if (isStopRequested())
        {
            emit finished(false, tr("Decompression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 5.0, tr("Preparing decryption..."));

        // 解压端同样统一转 UTF-8 后再交给库层
        const std::filesystem::path inputFilePath = EncodingUtils::qStringToPath(m_decompressInputFile);
        const std::filesystem::path outputDirectoryPath = EncodingUtils::qStringToPath(m_decompressOutputDir);
        const std::string inputFilePathUtf8 = EncodingUtils::pathToUtf8(inputFilePath);
        const std::string outputDirectoryUtf8 = EncodingUtils::pathToUtf8(outputDirectoryPath);

        if (isStopRequested())
        {
            emit finished(false, tr("Decompression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 8.0, tr("Reading archive header..."));

        // 读取定长文件头，识别归档格式与创建时采用的策略（决定是否需要口令）
        std::ifstream probeFile(inputFilePath, std::ios::binary);
        if (!probeFile)
        {
            throw std::runtime_error("Failed to open archive file for header reading");
        }

        Y_flib::DataBlock headerBuf(Y_flib::Constants::HEADER_SIZE);
        probeFile.read(reinterpret_cast<char *>(headerBuf.data()), Y_flib::Constants::HEADER_SIZE);
        if (!probeFile)
        {
            throw std::runtime_error("Failed to read archive header");
        }
        probeFile.close();

        Y_flib::Header fileHeader;
        std::memcpy(&fileHeader, headerBuf.data(), sizeof(Y_flib::Header));

        // 魔数不符：不是本程序生成的 .sy 文件（或文件已损坏）
        if (fileHeader.magicNum_1 != Y_flib::Constants::MAGIC_NUM ||
            fileHeader.magicNum_2 != Y_flib::Constants::MAGIC_NUM)
        {
            throw std::runtime_error("Invalid archive file format");
        }

        // 从头部恢复策略模式，决定后续如何解密/解压
        const Y_flib::CompressionMode detectedMode = Y_flib::StrategyFactory::idToMode(fileHeader.strategy);

        // 加密包却没给口令：不进入主循环，交给界面提示用户补口令后重试
        if (Y_flib::StrategyFactory::hasEncryption(detectedMode) && m_decompressPassword.isEmpty())
        {
            emit finished(false, QStringLiteral("This archive requires a password. Please enter the decryption key."));
            return;
        }

        if (isStopRequested())
        {
            emit finished(false, tr("Decompression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 10.0, tr("Creating strategy modules..."));

        auto modules = Y_flib::StrategyFactory::createModules(
            detectedMode,
            EncodingUtils::qStringToUtf8(m_decompressPassword));

        if (isStopRequested())
        {
            emit finished(false, tr("Decompression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 15.0, tr("Starting decompression..."));

        DecompressionLoop decompressor(inputFilePathUtf8, outputDirectoryUtf8);
        decompressor.setProgressCallback([this](const std::string &filename,
                                                double fileProgress,
                                                double overallProgress,
                                                const std::string &status) {
            if (isStopRequested())
            {
                // 与压缩主循环相同：以异常方式从循环中跳出
                throw std::runtime_error("Operation cancelled by user");
            }

            // 库层进度 0~100 线性映射到任务刻度 15~95
            const double mappedProgress = 15.0 + overallProgress * 0.80;
            if (shouldEmitProgress(mappedProgress))
            {
                emit detailedProgress(EncodingUtils::utf8ToQString(filename),
                                      fileProgress,
                                      mappedProgress,
                                      EncodingUtils::utf8ToQString(status));
            }
        });
        decompressor.decompressionLoop(*modules.encryption, *modules.compression);

        if (isStopRequested())
        {
            emit finished(false, tr("Decompression cancelled by user"));
            return;
        }

        emit detailedProgress("", 100.0, 100.0, tr("Completed"));
        emit finished(true, QStringLiteral("Decompression successful!\nOutput directory: %1")
                                 .arg(EncodingUtils::pathToQString(outputDirectoryPath)));
    }
    catch (const std::exception &e)
    {
        // 按约定文本识别“用户取消”，避免被当作错误上报
        if (std::string(e.what()) == "Operation cancelled by user")
        {
            emit finished(false, QStringLiteral("Decompression cancelled by user"));
        }
        else
        {
            emit finished(false, QStringLiteral("Decompression failed: %1\n\nPossible reasons:\n"
                                                "1. Incorrect decryption key\n"
                                                "2. Corrupted or incompatible .sy file\n"
                                                "3. Insufficient disk space")
                                     .arg(EncodingUtils::utf8ToQString(e.what())));
        }
    }
    catch (...)
    {
        emit finished(false, QStringLiteral("Decompression failed due to unknown error"));
    }
}
