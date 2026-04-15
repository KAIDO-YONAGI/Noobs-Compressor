#include "CompressionWorker.h"
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

bool CompressionWorker::validateCompressionParams()
{
    for (const QString &file : m_filesToCompress) {
        if (!QFileInfo::exists(file)) {
            emit finished(false, tr("File not found: %1").arg(file));
            return false;
        }
    }

    if (!QFileInfo(m_outputDir).isDir()) {
        emit finished(false, tr("Output directory not found: %1").arg(m_outputDir));
        return false;
    }

    return true;
}

bool CompressionWorker::validateDecompressionParams()
{
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

        // 使用QDir::toNativeSeparators统一路径分隔符
        std::vector<std::string> filePathToScan;
        for (const QString &file : m_filesToCompress) {
            QString nativeFile = QDir::toNativeSeparators(file);
            filePathToScan.push_back(nativeFile.toStdString());
            std::cout << "DEBUG: File path [" << filePathToScan.size() << "]: " << nativeFile.toStdString() << std::endl;
        }

        // 构建输出目录 - 统一使用反斜杠
        QString nativeOutputDir = QDir::toNativeSeparators(m_outputDir);
        std::string outputDirectory = nativeOutputDir.toStdString();

        // 确保输出目录以反斜杠结尾
        if (!outputDirectory.empty() && outputDirectory.back() != '\\') {
            outputDirectory += '\\';
        }
        std::cout << "DEBUG: Output directory: " << outputDirectory << std::endl;

        // 构建输出文件名
        std::string outputFileName = m_outputFileName.toStdString();

        // 移除可能的.sy后缀
        if (outputFileName.size() > 3 && outputFileName.substr(outputFileName.size() - 3) == ".sy") {
            outputFileName = outputFileName.substr(0, outputFileName.size() - 3);
        }

        // 完整的输出文件路径
        std::string compressionFilePath = outputDirectory + outputFileName + ".sy";
        std::cout << "DEBUG: Compression file path: " << compressionFilePath << std::endl;

        // 逻辑根
        std::string logicalRoot = outputFileName;

        emit progressChanged(20, tr("Creating AES encryption object..."));

        // 创建AES对象
        Aes aes(m_password.toStdString().c_str());

        emit progressChanged(30, tr("Writing file header..."));

        // 创建HeaderWriter
        HeaderWriter headerWriter_v0;
        headerWriter_v0.headerWriter(filePathToScan, compressionFilePath, logicalRoot);

        std::cout << "DEBUG: HeaderWriter done" << std::endl;

        emit progressChanged(40, tr("Starting compression..."));

        // 压缩
        CompressionLoop compressor(compressionFilePath);
        compressor.compressionLoop(filePathToScan, aes);

        emit progressChanged(90, tr("Associating icon..."));

        try {
            IconHandler::AssociateIconToSyFile(compressionFilePath, "");
        } catch (...) {}

        emit progressChanged(100, tr("Compression completed!"));
        emit finished(true, tr("Compression successful!\nOutput file: %1").arg(QString::fromStdString(compressionFilePath)));

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

        std::string inputFilePath = m_decompressInputFile.toStdString();
        std::string outputDirectory = m_decompressOutputDir.toStdString();

        emit progressChanged(20, tr("Creating AES decryption object..."));

        Aes aes(m_decompressPassword.toStdString().c_str());

        emit progressChanged(30, tr("Starting decompression..."));

        DecompressionLoop decompressor(inputFilePath, outputDirectory);
        decompressor.decompressionLoop(aes);

        emit progressChanged(100, tr("Decompression completed!"));
        emit finished(true, tr("Decompression successful!\nOutput directory: %1").arg(QString::fromStdString(outputDirectory)));

    } catch (const std::exception &e) {
        emit finished(false, tr("Decompression failed: %1\n\nPossible reasons:\n"
                                "1. Incorrect decryption key\n"
                                "2. Corrupted or incompatible .sy file\n"
                                "3. Insufficient disk space").arg(QString::fromStdString(e.what())));
    } catch (...) {
        emit finished(false, tr("Decompression failed due to unknown error"));
    }
}
