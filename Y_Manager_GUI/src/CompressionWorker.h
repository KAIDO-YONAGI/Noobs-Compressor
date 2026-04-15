#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>
#include <vector>
#include <string>

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
};
