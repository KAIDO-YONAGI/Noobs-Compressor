#include "CompressionWorker.h"


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
    emit detailedProgress("", 0.0, 0.0, tr("Validating parameters..."));

    if (!validateCompressionParams()) {
        return;
    }

    try {
        emit detailedProgress("", 0.0, 5.0, tr("Preparing files..."));

        // 使用QDir::toNativeSeparators统一路径分隔符
        // 重要：使用 toUtf8() 确保中文路径正确编码
        std::vector<std::string> filePathToScan;
        for (const QString &file : m_filesToCompress) {
            QString nativeFile = QDir::toNativeSeparators(file);
            filePathToScan.push_back(nativeFile.toUtf8().toStdString());
            std::cout << "DEBUG: File path [" << filePathToScan.size() << "]: " << nativeFile.toUtf8().toStdString() << std::endl;
        }

        // 构建输出目录 - 统一使用反斜杠，使用 UTF-8 编码
        QString nativeOutputDir = QDir::toNativeSeparators(m_outputDir);
        std::string outputDirectory = nativeOutputDir.toUtf8().toStdString();

        // 确保输出目录以反斜杠结尾
        if (!outputDirectory.empty() && outputDirectory.back() != '\\') {
            outputDirectory += '\\';
        }
        std::cout << "DEBUG: Output directory: " << outputDirectory << std::endl;

        // 构建输出文件名（UTF-8编码）
        std::string outputFileName = m_outputFileName.toUtf8().toStdString();

        // 移除可能的.sy后缀
        if (outputFileName.size() > 3 && outputFileName.substr(outputFileName.size() - 3) == ".sy") {
            outputFileName = outputFileName.substr(0, outputFileName.size() - 3);
        }

        // 完整的输出文件路径
        std::string compressionFilePath = outputDirectory + outputFileName + ".sy";
        std::cout << "DEBUG: Compression file path: " << compressionFilePath << std::endl;

        // 逻辑根
        std::string logicalRoot = outputFileName;

        emit detailedProgress("", 0.0, 10.0, tr("Creating AES encryption object..."));

        // 创建AES对象（UTF-8编码）
        Aes aes(m_password.toUtf8().toStdString().c_str());

        emit detailedProgress("", 0.0, 15.0, tr("Writing file header..."));

        // 创建HeaderWriter
        HeaderWriter headerWriter_v0;
        headerWriter_v0.headerWriter(filePathToScan, compressionFilePath, logicalRoot);

        std::cout << "DEBUG: HeaderWriter done" << std::endl;

        emit detailedProgress("", 0.0, 20.0, tr("Starting compression..."));

        // 压缩 - 设置进度回调
        CompressionLoop compressor(compressionFilePath);
        compressor.setProgressCallback([this](const std::string &filename, double fileProgress, double overallProgress, const std::string &status) {
            QString qFilename = QString::fromStdString(filename);
            QString qStatus = QString::fromStdString(status);
            // 将整体进度映射到20%-95%的范围（预留5%给图标关联）
            double mappedProgress = 20.0 + overallProgress * 0.75;
            emit detailedProgress(qFilename, fileProgress, mappedProgress, qStatus);
        });
        compressor.compressionLoop(filePathToScan, aes);

        emit detailedProgress("", 100.0, 95.0, tr("Associating icon..."));

        try {
            IconHandler::AssociateIconToSyFile(compressionFilePath, "");
        } catch (...) {}

        emit detailedProgress("", 100.0, 100.0, tr("Completed"));
        emit finished(true, tr("Compression successful!\nOutput file: %1").arg(QString::fromUtf8(compressionFilePath.c_str())));

    } catch (const std::exception &e) {
        emit finished(false, tr("Compression failed: %1").arg(QString::fromStdString(e.what())));
    } catch (...) {
        emit finished(false, tr("Compression failed due to unknown error"));
    }
}

void CompressionWorker::doDecompression()
{
    emit detailedProgress("", 0.0, 0.0, tr("Validating parameters..."));

    if (!validateDecompressionParams()) {
        return;
    }

    try {
        emit detailedProgress("", 0.0, 5.0, tr("Preparing decryption..."));

        std::string inputFilePath = m_decompressInputFile.toUtf8().toStdString();
        std::string outputDirectory = m_decompressOutputDir.toUtf8().toStdString();

        emit detailedProgress("", 0.0, 10.0, tr("Creating AES decryption object..."));

        Aes aes(m_decompressPassword.toUtf8().toStdString().c_str());

        emit detailedProgress("", 0.0, 15.0, tr("Starting decompression..."));

        // 解压 - 设置进度回调
        DecompressionLoop decompressor(inputFilePath, outputDirectory);
        decompressor.setProgressCallback([this](const std::string &filename, double fileProgress, double overallProgress, const std::string &status) {
            QString qFilename = QString::fromStdString(filename);
            QString qStatus = QString::fromStdString(status);
            // 将整体进度映射到15%-95%的范围
            double mappedProgress = 15.0 + overallProgress * 0.80;
            emit detailedProgress(qFilename, fileProgress, mappedProgress, qStatus);
        });
        decompressor.decompressionLoop(aes);

        emit detailedProgress("", 100.0, 100.0, tr("Completed"));
        emit finished(true, tr("Decompression successful!\nOutput directory: %1").arg(QString::fromUtf8(outputDirectory.c_str())));

    } catch (const std::exception &e) {
        emit finished(false, tr("Decompression failed: %1\n\nPossible reasons:\n"
                                "1. Incorrect decryption key\n"
                                "2. Corrupted or incompatible .sy file\n"
                                "3. Insufficient disk space").arg(QString::fromStdString(e.what())));
    } catch (...) {
        emit finished(false, tr("Decompression failed due to unknown error"));
    }
}
