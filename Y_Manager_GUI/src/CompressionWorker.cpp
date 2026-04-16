#include "CompressionWorker.h"


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
    emit detailedProgress("", 0.0, 0.0, tr("Validating parameters..."));

    if (!validateCompressionParams()) {
        return;
    }

    try {
        emit detailedProgress("", 0.0, 5.0, tr("Preparing files..."));

        // 使用UTF-8编码的路径字符串
        std::vector<std::string> filePathToScan;
        for (const QString &file : m_filesToCompress) {
            std::string utf8Path = file.toUtf8().toStdString();
            filePathToScan.push_back(utf8Path);
            std::cout << "DEBUG: File path [" << filePathToScan.size() << "]: " << utf8Path << std::endl;
        }

        // 构建输出目录路径（使用UTF-8编码）
        std::string outputDirectory = m_outputDir.toUtf8().toStdString();

        // 确保输出目录以反斜杠结尾
        if (!outputDirectory.empty() && outputDirectory.back() != '\\' && outputDirectory.back() != '/') {
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
