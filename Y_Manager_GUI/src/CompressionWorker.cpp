#include "CompressionWorker.h"
#include <cstring>


// 节流逻辑：限制信号发送频率
bool CompressionWorker::shouldEmitProgress(double currentProgress)
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastProgressTime).count();

    // 条件：时间间隔足够 或 进度变化足够 或 完成时
    bool shouldEmit = (elapsed >= PROGRESS_INTERVAL_MS) ||
                      (currentProgress - m_lastEmittedProgress >= PROGRESS_DELTA) ||
                      (currentProgress >= 100.0);

    if (shouldEmit) {
        m_lastProgressTime = now;
        m_lastEmittedProgress = currentProgress;
    }
    return shouldEmit;
}

// Windows API路径处理辅助函数（参考命令行版本）
static std::filesystem::path make_path(const std::string &utf8_str)
{
    if (utf8_str.empty())
    {
        return std::filesystem::path("");
    }

    // 使用 UTF-8 进行转换
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);

    // 如果 UTF-8 转换失败，尝试用本地代码页转换（ANSI）
    if (wide_len == 0)
    {
        wide_len = MultiByteToWideChar(CP_ACP, 0, utf8_str.c_str(), -1, nullptr, 0);
        if (wide_len == 0)
        {
            throw std::runtime_error("Failed to convert string to wide string");
        }

        std::wstring wide_str(wide_len, L'\0');
        MultiByteToWideChar(CP_ACP, 0, utf8_str.c_str(), -1, &wide_str[0], wide_len);

        if (!wide_str.empty() && wide_str.back() == L'\0')
        {
            wide_str.pop_back();
        }

        return std::filesystem::path(wide_str);
    }

    // UTF-8 转换成功
    std::wstring wide_str(wide_len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wide_str[0], wide_len);

    if (!wide_str.empty() && wide_str.back() == L'\0')
    {
        wide_str.pop_back();
    }

    return std::filesystem::path(wide_str);
}

// 将宽字符路径转换为UTF-8字符串
static std::string wide_to_utf8(const std::wstring &wide_str)
{
    if (wide_str.empty())
    {
        return "";
    }

    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8_len == 0)
    {
        return "";
    }

    std::string utf8_str(utf8_len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, &utf8_str[0], utf8_len, nullptr, nullptr);

    if (!utf8_str.empty() && utf8_str.back() == '\0')
    {
        utf8_str.pop_back();
    }

    return utf8_str;
}

CompressionWorker::CompressionWorker(QObject *parent)
    : QObject(parent)
    , m_isDecompression(false)
    , m_mode(Y_flib::CompressionMode::HuffmanAES)
{
}

CompressionWorker::~CompressionWorker()
{
}

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
        // 使用Windows API验证路径存在性
        std::string utf8Path = file.toUtf8().toStdString();
        try {
            auto path = make_path(utf8Path);
            if (!std::filesystem::exists(path)) {
                emit finished(false, tr("File not found: %1").arg(file));
                return false;
            }
        } catch (...) {
            emit finished(false, tr("Invalid path: %1").arg(file));
            return false;
        }
    }

    // 验证输出目录
    std::string utf8OutputDir = m_outputDir.toUtf8().toStdString();
    try {
        auto outputPath = make_path(utf8OutputDir);
        if (!std::filesystem::is_directory(outputPath)) {
            emit finished(false, tr("Output directory not found: %1").arg(m_outputDir));
            return false;
        }
    } catch (...) {
        emit finished(false, tr("Invalid output directory: %1").arg(m_outputDir));
        return false;
    }

    return true;
}

bool CompressionWorker::validateDecompressionParams()
{
    // 使用Windows API验证路径存在性
    std::string utf8Path = m_decompressInputFile.toUtf8().toStdString();
    try {
        auto path = make_path(utf8Path);
        if (!std::filesystem::exists(path)) {
            emit finished(false, tr("Archive file not found: %1").arg(m_decompressInputFile));
            return false;
        }
    } catch (...) {
        emit finished(false, tr("Invalid archive path: %1").arg(m_decompressInputFile));
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
    resetStopFlag();  // 重置停止标志
    emit detailedProgress("", 0.0, 0.0, tr("Validating parameters..."));

    if (!validateCompressionParams()) {
        return;
    }

    try {
        if (isStopRequested()) {
            emit finished(false, tr("Compression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 5.0, tr("Preparing files..."));

        // 使用UTF-8编码的路径字符串
        std::vector<std::string> filePathToScan;
        for (const QString &file : m_filesToCompress) {
            std::string utf8Path = file.toUtf8().toStdString();
            filePathToScan.push_back(utf8Path);
        }

        // 构建输出目录路径（使用UTF-8编码）
        std::string outputDirectory = m_outputDir.toUtf8().toStdString();

        // 确保输出目录以反斜杠结尾
        if (!outputDirectory.empty() && outputDirectory.back() != '\\' && outputDirectory.back() != '/') {
            outputDirectory += '\\';
        }

        // 构建输出文件名（UTF-8编码）
        std::string outputFileName = m_outputFileName.toUtf8().toStdString();

        // 移除可能的.sy后缀
        if (outputFileName.size() > 3 && outputFileName.substr(outputFileName.size() - 3) == ".sy") {
            outputFileName = outputFileName.substr(0, outputFileName.size() - 3);
        }

        // 完整的输出文件路径
        std::string compressionFilePath = outputDirectory + outputFileName + ".sy";

        // 逻辑根
        std::string logicalRoot = outputFileName;

        if (isStopRequested()) {
            emit finished(false, tr("Compression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 10.0, tr("Creating strategy modules..."));

        // 使用策略工厂创建压缩和加密模块
        auto modules = Y_flib::StrategyFactory::createModules(m_mode, m_password.toUtf8().toStdString());

        if (isStopRequested()) {
            emit finished(false, tr("Compression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 15.0, tr("Writing file header..."));

        // 创建HeaderWriter，传入策略模式
        HeaderWriter headerWriter_v0;
        headerWriter_v0.headerWriter(filePathToScan, compressionFilePath, logicalRoot, m_mode);

        if (isStopRequested()) {
            emit finished(false, tr("Compression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 20.0, tr("Starting compression..."));

        // 压缩 - 设置进度回调，包含停止检查和节流
        CompressionLoop compressor(compressionFilePath);
        compressor.setProgressCallback([this](const std::string &filename, double fileProgress, double overallProgress, const std::string &status) {
            if (isStopRequested()) {
                throw std::runtime_error("Operation cancelled by user");
            }
            // 将整体进度映射到20%-95%的范围（预留5%给图标关联）
            double mappedProgress = 20.0 + overallProgress * 0.75;

            // 节流：限制信号发送频率
            if (shouldEmitProgress(mappedProgress)) {
                QString qFilename = QString::fromStdString(filename);
                QString qStatus = QString::fromStdString(status);
                emit detailedProgress(qFilename, fileProgress, mappedProgress, qStatus);
            }
        });
        compressor.compressionLoop(filePathToScan, *modules.encryption, *modules.compression, m_mode);

        if (isStopRequested()) {
            emit finished(false, tr("Compression cancelled by user"));
            return;
        }

        // 关联图标到 .sy 文件
        try {
            IconHandler::AssociateIconToSyFile(compressionFilePath, "");
        } catch (...) {}

        emit detailedProgress("", 100.0, 100.0, tr("Completed"));
        emit finished(true, tr("Compression successful!\nOutput file: %1").arg(QString::fromUtf8(compressionFilePath.c_str())));

    } catch (const std::exception &e) {
        if (std::string(e.what()) == "Operation cancelled by user") {
            emit finished(false, tr("Compression cancelled by user"));
        } else {
            emit finished(false, tr("Compression failed: %1").arg(QString::fromStdString(e.what())));
        }
    } catch (...) {
        emit finished(false, tr("Compression failed due to unknown error"));
    }
}

void CompressionWorker::doDecompression()
{
    resetStopFlag();  // 重置停止标志
    emit detailedProgress("", 0.0, 0.0, tr("Validating parameters..."));

    if (!validateDecompressionParams()) {
        return;
    }

    try {
        if (isStopRequested()) {
            emit finished(false, tr("Decompression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 5.0, tr("Preparing decryption..."));

        std::string inputFilePath = m_decompressInputFile.toUtf8().toStdString();
        std::string outputDirectory = m_decompressOutputDir.toUtf8().toStdString();

        if (isStopRequested()) {
            emit finished(false, tr("Decompression cancelled by user"));
            return;
        }

        // 从文件头读取策略号
        emit detailedProgress("", 0.0, 8.0, tr("Reading archive header..."));
        PathTransfer transfer;
        std::ifstream probeFile(transfer.transPath(inputFilePath), std::ios::binary);
        if (!probeFile)
            throw std::runtime_error("Failed to open archive file for header reading");

        Y_flib::DataBlock headerBuf(Y_flib::Constants::HEADER_SIZE);
        probeFile.read(reinterpret_cast<char*>(headerBuf.data()), Y_flib::Constants::HEADER_SIZE);
        if (!probeFile)
            throw std::runtime_error("Failed to read archive header");
        probeFile.close();

        Y_flib::Header fileHeader;
        std::memcpy(&fileHeader, headerBuf.data(), sizeof(Y_flib::Header));

        if (fileHeader.magicNum_1 != Y_flib::Constants::MAGIC_NUM ||
            fileHeader.magicNum_2 != Y_flib::Constants::MAGIC_NUM)
        {
            throw std::runtime_error("Invalid archive file format");
        }

        Y_flib::CompressionMode detectedMode = Y_flib::StrategyFactory::idToMode(fileHeader.strategy);

        if (Y_flib::StrategyFactory::hasEncryption(detectedMode) && m_decompressPassword.isEmpty()) {
            emit finished(false, tr("This archive requires a password. Please enter the decryption key."));
            return;
        }

        if (isStopRequested()) {
            emit finished(false, tr("Decompression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 10.0, tr("Creating strategy modules..."));

        // 使用策略工厂根据文件头创建对应模块
        auto modules = Y_flib::StrategyFactory::createModules(detectedMode, m_decompressPassword.toUtf8().toStdString());

        if (isStopRequested()) {
            emit finished(false, tr("Decompression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 15.0, tr("Starting decompression..."));

        // 解压 - 设置进度回调，包含停止检查和节流
        DecompressionLoop decompressor(inputFilePath, outputDirectory);
        decompressor.setProgressCallback([this](const std::string &filename, double fileProgress, double overallProgress, const std::string &status) {
            if (isStopRequested()) {
                throw std::runtime_error("Operation cancelled by user");
            }
            // 将整体进度映射到15%-95%的范围
            double mappedProgress = 15.0 + overallProgress * 0.80;

            // 节流：限制信号发送频率
            if (shouldEmitProgress(mappedProgress)) {
                QString qFilename = QString::fromStdString(filename);
                QString qStatus = QString::fromStdString(status);
                emit detailedProgress(qFilename, fileProgress, mappedProgress, qStatus);
            }
        });
        decompressor.decompressionLoop(*modules.encryption, *modules.compression);

        if (isStopRequested()) {
            emit finished(false, tr("Decompression cancelled by user"));
            return;
        }

        emit detailedProgress("", 100.0, 100.0, tr("Completed"));
        emit finished(true, tr("Decompression successful!\nOutput directory: %1").arg(QString::fromUtf8(outputDirectory.c_str())));

    } catch (const std::exception &e) {
        if (std::string(e.what()) == "Operation cancelled by user") {
            emit finished(false, tr("Decompression cancelled by user"));
        } else {
            emit finished(false, tr("Decompression failed: %1\n\nPossible reasons:\n"
                                    "1. Incorrect decryption key\n"
                                    "2. Corrupted or incompatible .sy file\n"
                                    "3. Insufficient disk space").arg(QString::fromStdString(e.what())));
        }
    } catch (...) {
        emit finished(false, tr("Decompression failed due to unknown error"));
    }
}
