#include "CompressionWorker.h"
#include "../EncryptionModules/Aes/include/My_Aes.h"
#include "../CompressorFileSystem/DataCommunication/include/HeaderWriter.h"
#include "../Y_Manager/MainLoop.h"
#include "../Y_Manager/IconHandler.h"
#include <QFileInfo>
#include <QDir>
#include <stdexcept>
#include <filesystem>

CompressionWorker::CompressionWorker(QObject *parent)
    : QObject(parent)
    , m_isDecompression(false)
{
}

CompressionWorker::~CompressionWorker()
{
}

void CompressionWorker::setCompressionParams(const QStringList &files,
                                              const QString &outputDir,
                                              const QString &fileName,
                                              const QString &password)
{
    m_filesToCompress = files;
    m_outputDir = outputDir;
    m_outputFileName = fileName;
    m_password = password;
    m_isDecompression = false;
}

void CompressionWorker::setDecompressionParams(const QString &inputFile,
                                                const QString &outputDir,
                                                const QString &password)
{
    m_decompressInputFile = inputFile;
    m_decompressOutputDir = outputDir;
    m_decompressPassword = password;
    m_isDecompression = true;
}

QString CompressionWorker::getStdString(const QString &qstr)
{
    return QDir::cleanPath(qstr);
}

bool CompressionWorker::validateCompressionParams()
{
    // 检查文件是否存在
    for (const QString &file : m_filesToCompress) {
        if (!QFileInfo::exists(file)) {
            emit finished(false, tr("File not found: %1").arg(file));
            return false;
        }
    }

    // 检查输出目录
    if (!QFileInfo(m_outputDir).isDir()) {
        emit finished(false, tr("Output directory not found: %1").arg(m_outputDir));
        return false;
    }

    return true;
}

bool CompressionWorker::validateDecompressionParams()
{
    // 检查输入文件
    if (!QFileInfo::exists(m_decompressInputFile)) {
        emit finished(false, tr("Archive file not found: %1").arg(m_decompressInputFile));
        return false;
    }

    if (!m_decompressInputFile.toLower().endsWith(".sy")) {
        emit finished(false, tr("Only .sy files can be decompressed"));
        return false;
    }

    return true;
}

void CompressionWorker::doCompression()
{
    emit progressChanged(0, tr("Validating parameters..."));

    if (!validateCompressionParams()) {
        return;
    }

    try {
        emit progressChanged(10, tr("Preparing files..."));

        // 转换文件列表
        std::vector<std::string> filePathToScan;
        for (const QString &file : m_filesToCompress) {
            filePathToScan.push_back(getStdString(file).toStdString());
        }

        // 构建输出路径
        QString outputDirClean = QDir::cleanPath(m_outputDir);
        QString fileName = m_outputFileName.trimmed();

        // 移除可能的.sy后缀
        if (fileName.toLower().endsWith(".sy")) {
            fileName.chop(3);
        }

        QString compressionFilePath = outputDirClean + "/" + fileName + ".sy";
        std::string compressionFilePathStd = compressionFilePath.toStdString();

        emit progressChanged(20, tr("Creating AES encryption object..."));

        // 创建AES对象
        Aes aes(m_password.toStdString().c_str());

        emit progressChanged(30, tr("Writing file header..."));

        // 创建HeaderWriter
        HeaderWriter headerWriter_v0;
        std::string logicalRoot = fileName.toStdString();
        headerWriter_v0.headerWriter(filePathToScan, compressionFilePathStd, logicalRoot);

        emit progressChanged(40, tr("Starting compression..."));

        // 压缩
        CompressionLoop compressor(compressionFilePathStd);
        compressor.compressionLoop(filePathToScan, aes);

        emit progressChanged(90, tr("Associating icon..."));

        // 关联图标（可选）
        try {
            IconHandler::AssociateIconToSyFile(compressionFilePathStd, "");
        } catch (...) {
            // 忽略图标关联错误
        }

        emit progressChanged(100, tr("Compression completed!"));
        emit finished(true, tr("Compression successful!\nOutput file: %1").arg(compressionFilePath));

    } catch (const std::exception &e) {
        emit finished(false, tr("Compression failed: %1").arg(QString::fromStdString(e.what())));
    } catch (...) {
        emit finished(false, tr("Compression failed due to unknown error"));
    }
}

void CompressionWorker::doDecompression()
{
    emit progressChanged(0, tr("Validating parameters..."));

    if (!validateDecompressionParams()) {
        return;
    }

    try {
        emit progressChanged(10, tr("Preparing decryption..."));

        // 转换路径
        std::string inputFilePath = getStdString(m_decompressInputFile).toStdString();
        std::string outputDirectory;

        if (!m_decompressOutputDir.isEmpty()) {
            outputDirectory = QDir::cleanPath(m_decompressOutputDir).toStdString();
        }

        emit progressChanged(20, tr("Creating AES decryption object..."));

        // 创建AES对象
        Aes aes(m_decompressPassword.toStdString().c_str());

        emit progressChanged(30, tr("Starting decompression..."));

        // 解压
        DecompressionLoop decompressor(inputFilePath, outputDirectory);
        decompressor.decompressionLoop(aes);

        emit progressChanged(100, tr("Decompression completed!"));
        emit finished(true, tr("Decompression successful!\nOutput directory: %1").arg(QString::fromStdString(outputDirectory)));

    } catch (const std::exception &e) {
        QString errorMsg = QString::fromStdString(e.what());
        emit finished(false, tr("Decompression failed: %1\n\nPossible reasons:\n"
                                "1. Incorrect decryption key\n"
                                "2. Corrupted or incompatible .sy file\n"
                                "3. Insufficient disk space").arg(errorMsg));
    } catch (...) {
        emit finished(false, tr("Decompression failed due to unknown error"));
    }
}