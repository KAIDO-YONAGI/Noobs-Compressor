#include "CompressionWorker.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

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

bool CompressionWorker::validateCompressionParams()
{
    for (const QString &file : m_filesToCompress)
    {
        try
        {
            if (!std::filesystem::exists(EncodingUtils::qStringToPath(file)))
            {
                emit finished(false, tr("File not found: %1").arg(file));
                return false;
            }
        }
        catch (...)
        {
            emit finished(false, tr("Invalid path: %1").arg(file));
            return false;
        }
    }

    try
    {
        const std::filesystem::path outputPath = EncodingUtils::qStringToPath(m_outputDir);
        if (!std::filesystem::is_directory(outputPath))
        {
            emit finished(false, tr("Output directory not found: %1").arg(m_outputDir));
            return false;
        }
    }
    catch (...)
    {
        emit finished(false, tr("Invalid output directory: %1").arg(m_outputDir));
        return false;
    }

    return true;
}

bool CompressionWorker::validateDecompressionParams()
{
    try
    {
        if (!std::filesystem::exists(EncodingUtils::qStringToPath(m_decompressInputFile)))
        {
            emit finished(false, tr("Archive file not found: %1").arg(m_decompressInputFile));
            return false;
        }
    }
    catch (...)
    {
        emit finished(false, tr("Invalid archive path: %1").arg(m_decompressInputFile));
        return false;
    }

    if (!m_decompressInputFile.toLower().endsWith(".sy"))
    {
        emit finished(false, tr("Only .sy files can be decompressed"));
        return false;
    }

    return true;
}

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

        std::vector<std::string> filePathToScan;
        filePathToScan.reserve(static_cast<size_t>(m_filesToCompress.size()));
        for (const QString &file : m_filesToCompress)
        {
            filePathToScan.push_back(EncodingUtils::qStringToUtf8(file));
        }

        std::string outputFileName = EncodingUtils::qStringToUtf8(m_outputFileName);
        if (outputFileName.size() > 3 && outputFileName.substr(outputFileName.size() - 3) == ".sy")
        {
            outputFileName.erase(outputFileName.size() - 3);
        }

        const std::filesystem::path compressionFilePath =
            EncodingUtils::qStringToPath(m_outputDir) / EncodingUtils::pathFromUtf8(outputFileName + ".sy");
        std::string compressionFilePathUtf8 = EncodingUtils::pathToUtf8(compressionFilePath);
        const std::string logicalRoot = outputFileName;

        if (isStopRequested())
        {
            emit finished(false, tr("Compression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 10.0, tr("Creating strategy modules..."));

        auto modules = Y_flib::StrategyFactory::createModules(m_mode, EncodingUtils::qStringToUtf8(m_password));

        if (isStopRequested())
        {
            emit finished(false, tr("Compression cancelled by user"));
            return;
        }

        emit detailedProgress("", 0.0, 15.0, tr("Writing file header..."));

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
                throw std::runtime_error("Operation cancelled by user");
            }

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

        try
        {
            IconHandler::AssociateIconToSyFile(compressionFilePathUtf8, "");
        }
        catch (...)
        {
        }

        emit detailedProgress("", 100.0, 100.0, tr("Completed"));
        emit finished(true, tr("Compression successful!\nOutput file: %1").arg(EncodingUtils::pathToQString(compressionFilePath)));
    }
    catch (const std::exception &e)
    {
        if (std::string(e.what()) == "Operation cancelled by user")
        {
            emit finished(false, tr("Compression cancelled by user"));
        }
        else
        {
            emit finished(false, tr("Compression failed: %1").arg(EncodingUtils::utf8ToQString(e.what())));
        }
    }
    catch (...)
    {
        emit finished(false, tr("Compression failed due to unknown error"));
    }
}

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

        if (fileHeader.magicNum_1 != Y_flib::Constants::MAGIC_NUM ||
            fileHeader.magicNum_2 != Y_flib::Constants::MAGIC_NUM)
        {
            throw std::runtime_error("Invalid archive file format");
        }

        const Y_flib::CompressionMode detectedMode = Y_flib::StrategyFactory::idToMode(fileHeader.strategy);

        if (Y_flib::StrategyFactory::hasEncryption(detectedMode) && m_decompressPassword.isEmpty())
        {
            emit finished(false, tr("This archive requires a password. Please enter the decryption key."));
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
                throw std::runtime_error("Operation cancelled by user");
            }

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
        emit finished(true, tr("Decompression successful!\nOutput directory: %1").arg(EncodingUtils::pathToQString(outputDirectoryPath)));
    }
    catch (const std::exception &e)
    {
        if (std::string(e.what()) == "Operation cancelled by user")
        {
            emit finished(false, tr("Decompression cancelled by user"));
        }
        else
        {
            emit finished(false, tr("Decompression failed: %1\n\nPossible reasons:\n"
                                    "1. Incorrect decryption key\n"
                                    "2. Corrupted or incompatible .sy file\n"
                                    "3. Insufficient disk space")
                                    .arg(EncodingUtils::utf8ToQString(e.what())));
        }
    }
    catch (...)
    {
        emit finished(false, tr("Decompression failed due to unknown error"));
    }
}
