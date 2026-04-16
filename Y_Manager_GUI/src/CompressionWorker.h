#pragma once
#include "../EncryptionModules/Aes/include/My_Aes.h"
#include "../CompressorFileSystem/DataCommunication/include/HeaderWriter.h"
#include "../Y_Manager/MainLoop.h"
#include "../Y_Manager/IconHandler.h"
#include <QFileInfo>
#include <QDir>
#include <stdexcept>
#include <filesystem>
#include <windows.h>
#include <iostream>
#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <chrono>

class CompressionWorker : public QObject
{
    Q_OBJECT

public:
    explicit CompressionWorker(QObject *parent = nullptr);
    ~CompressionWorker();

    // 设置压缩参数
    void setCompressionParams(const QStringList &files,
                               const QString &outputDir,
                               const QString &fileName,
                               const QString &password);

    // 设置解压参数
    void setDecompressionParams(const QString &inputFile,
                                 const QString &outputDir,
                                 const QString &password);

    // 请求停止工作
    void requestStop() { m_stopRequested.store(true); }

    // 检查是否请求停止
    bool isStopRequested() const { return m_stopRequested.load(); }

    // 重置停止标志
    void resetStopFlag() { m_stopRequested.store(false); }

public slots:
    void doCompression();
    void doDecompression();

signals:
    // 详细进度信号: (当前文件名, 当前文件进度百分比, 整体进度百分比, 状态消息)
    void detailedProgress(const QString &filename, double fileProgress, double overallProgress, const QString &status);
    void finished(bool success, const QString &message);

private:
    bool validateCompressionParams();
    bool validateDecompressionParams();
    QString getStdString(const QString &qstr);

    // 节流发送进度信号
    bool shouldEmitProgress(double currentProgress);

    // 压缩参数
    QStringList m_filesToCompress;
    QString m_outputDir;
    QString m_outputFileName;
    QString m_password;

    // 解压参数
    QString m_decompressInputFile;
    QString m_decompressOutputDir;
    QString m_decompressPassword;

    // 状态
    bool m_isDecompression;

    // 停止请求标志
    std::atomic<bool> m_stopRequested{false};

    // 进度信号节流
    std::chrono::steady_clock::time_point m_lastProgressTime;
    double m_lastEmittedProgress{-1.0};
    static constexpr int PROGRESS_INTERVAL_MS = 200;  // 最小信号间隔
    static constexpr double PROGRESS_DELTA = 2.0;     // 最小进度变化
};
