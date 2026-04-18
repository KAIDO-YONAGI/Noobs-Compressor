#pragma once

#include "../CompressorFileSystem/DataCommunication/include/HeaderWriter.h"
#include "../CompressorFileSystem/DataCommunication/include/StrategyFactory.h"
#include "../CompressorFileSystem/DataCommunication/include/ToolClasses.h"
#include "../IconHandler.h"
#include "../Y_Manager/MainLoop.h"

#include <QObject>
#include <QString>
#include <QStringList>

#include <atomic>
#include <chrono>

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

    void setCompressionParams(const QStringList &files,
                              const QString &outputDir,
                              const QString &fileName,
                              const QString &password,
                              Y_flib::CompressionMode mode = Y_flib::CompressionMode::HuffmanAES);

    void setDecompressionParams(const QString &inputFile,
                                const QString &outputDir,
                                const QString &password);

    void requestStop() { m_stopRequested.store(true); }
    bool isStopRequested() const { return m_stopRequested.load(); }

    void resetStopFlag()
    {
        m_stopRequested.store(false);
        m_lastProgressTime = std::chrono::steady_clock::now();
        m_lastEmittedProgress = -1.0;
    }

public slots:
    void doCompression();
    void doDecompression();

signals:
    void detailedProgress(const QString &filename, double fileProgress, double overallProgress, const QString &status);
    void finished(bool success, const QString &message);

private:
    bool validateCompressionParams();
    bool validateDecompressionParams();
    bool shouldEmitProgress(double currentProgress);

    QStringList m_filesToCompress;
    QString m_outputDir;
    QString m_outputFileName;
    QString m_password;
    Y_flib::CompressionMode m_mode{Y_flib::CompressionMode::HuffmanAES};

    QString m_decompressInputFile;
    QString m_decompressOutputDir;
    QString m_decompressPassword;

    std::atomic<bool> m_stopRequested{false};
    std::chrono::steady_clock::time_point m_lastProgressTime;
    double m_lastEmittedProgress{-1.0};

    static constexpr int PROGRESS_INTERVAL_MS = 200;
    static constexpr double PROGRESS_DELTA = 2.0;
};
