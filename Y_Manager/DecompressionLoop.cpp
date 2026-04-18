#include "DecompressionLoop.h"
#include <chrono>
#include <memory>

// 进度回调最小间隔（毫秒）
static constexpr int PROGRESS_CALLBACK_INTERVAL_MS = 100;

void DecompressionLoop::decompressionLoop(Y_flib::IEncryption &encryption, Y_flib::ICompression &compression)
{
    m_encryption = &encryption;
    m_compression = &compression;

    std::vector<std::string> blank;
    BinaryStandardLoader headerLoaderIterator(fullPath.string(), blank, parentPath);
    headerLoaderIterator.headerLoaderIterator(encryption);
    Y_flib::DirectoryOffsetSize dataOffset = headerLoaderIterator.getDirectoryOffset();

    Locator locator;

    // 进度回调节流
    auto lastCallbackTime = std::chrono::steady_clock::now();
    double lastReportedProgress = -1.0;

    // 计算总文件数
    m_totalFiles = 0;
    m_processedFiles = 0;
    m_totalFiles = headerLoaderIterator.fileQueue.size();

    while (!headerLoaderIterator.allLoopIsDone())
    {
        processDirectories(headerLoaderIterator);

        while (!headerLoaderIterator.fileQueue.empty())
        {
            processFile(headerLoaderIterator, locator, dataOffset, lastCallbackTime, lastReportedProgress);
        }

        while (headerLoaderIterator.fileQueue.empty() && !headerLoaderIterator.allLoopIsDone())
        {
            headerLoaderIterator.restartLoader();
            headerLoaderIterator.headerLoaderIterator(encryption);
            m_totalFiles += headerLoaderIterator.fileQueue.size();
        }
    }

    // 完成回调
    if (m_progressCallback)
    {
        m_progressCallback("", 100.0, 100.0, "Completed");
    }
}

void DecompressionLoop::processDirectories(BinaryStandardLoader &headerLoaderIterator)
{
    while (!headerLoaderIterator.directoryQueue_ready.empty())
    {
        std::filesystem::path dirToCreate = headerLoaderIterator.directoryQueue_ready.front();

        if (!dirToCreate.is_absolute())
        {
            dirToCreate = parentPath / dirToCreate;
        }

        createDirectory(dirToCreate);
        headerLoaderIterator.directoryQueue_ready.pop();
    }
}

void DecompressionLoop::processFile(
    BinaryStandardLoader &headerLoaderIterator,
    Locator &locator,
    Y_flib::DirectoryOffsetSize &dataOffset,
    std::chrono::steady_clock::time_point &lastCallbackTime,
    double &lastReportedProgress)
{
    std::filesystem::path relativePath = headerLoaderIterator.fileQueue.front().first.getFullPath();
    std::filesystem::path fullFilePath = parentPath / relativePath;

    createFile(fullFilePath);
    std::ifstream &inFile = headerLoaderIterator.getInFile();

    locator.locateFromBegin(inFile, dataOffset);

    std::filesystem::path filename = fullFilePath.filename();

    // 进度回调 - 开始处理文件
    if (m_progressCallback)
    {
        double overallProgress = m_totalFiles > 0 ? (100.0 * m_processedFiles / m_totalFiles) : 0;
        m_progressCallback(filename.string(), 0.0, overallProgress, "Decompressing");
        lastCallbackTime = std::chrono::steady_clock::now();
    }

    DataExporter dataExporter(fullFilePath);
    Y_flib::FileSize fileCompressedSize = headerLoaderIterator.fileQueue.front().second;
    Y_flib::FileSize originalSize = headerLoaderIterator.fileQueue.front().first.getFileSizeInDetails();
    Y_flib::FileSize totalDecompressedBytes = 0;

    // 预分配缓冲区
    Y_flib::DataBlock rawMetadata;
    Y_flib::DataBlock decryptedMetadata;
    Y_flib::DataBlock rawData;
    Y_flib::DataBlock decryptedData;
    Y_flib::DataBlock decompressedData;
    rawMetadata.reserve(Y_flib::Constants::BUFFER_SIZE);
    decryptedMetadata.reserve(Y_flib::Constants::BUFFER_SIZE);
    rawData.reserve(Y_flib::Constants::BUFFER_SIZE);
    decryptedData.reserve(Y_flib::Constants::BUFFER_SIZE);
    decompressedData.reserve(Y_flib::Constants::BUFFER_SIZE * 2);

    // 处理文件的每个块
    while (totalDecompressedBytes < originalSize && fileCompressedSize > 0)
    {
        processDataBlock(inFile, fullFilePath, *m_encryption, *m_compression,
                         rawMetadata, decryptedMetadata, rawData, decryptedData, decompressedData,
                         fileCompressedSize, totalDecompressedBytes, originalSize, dataExporter);

        // 进度回调
        reportProgress(filename, totalDecompressedBytes, originalSize, lastCallbackTime, lastReportedProgress);
    }

    dataOffset = inFile.tellg();
    headerLoaderIterator.fileQueue.pop();
    m_processedFiles++;

    // 进度回调 - 文件完成
    if (m_progressCallback)
    {
        double overallProgress = m_totalFiles > 0 ? (100.0 * m_processedFiles / m_totalFiles) : 100.0;
        m_progressCallback(filename.string(), 100.0, overallProgress, "Decompressing");
        lastCallbackTime = std::chrono::steady_clock::now();
    }
}

void DecompressionLoop::processDataBlock(
    std::ifstream &inFile,
    const std::filesystem::path &filePath,
    Y_flib::IEncryption &encryption,
    Y_flib::ICompression &compression,
    Y_flib::DataBlock &rawMetadata,
    Y_flib::DataBlock &decryptedMetadata,
    Y_flib::DataBlock &rawData,
    Y_flib::DataBlock &decryptedData,
    Y_flib::DataBlock &decompressedData,
    Y_flib::FileSize &fileCompressedSize,
    Y_flib::FileSize &totalDecompressedBytes,
    Y_flib::FileSize originalSize,
    DataExporter &dataExporter)
{
    StandardsReader numReader(inFile);
    DataLoader loader(filePath);

    // 读取分隔标志
    if (!(numReader.readBinaryStandards<Y_flib::FlagType>() == Y_flib::FlagType::Separated))
        throw std::runtime_error("decompressionLoop()-Error:Can't read SEPARATED_FLAG before metadata block");

    // 读取 metadata 块
    Y_flib::DirectoryOffsetSize metadataBlockSize = numReader.readBinaryStandards<Y_flib::DirectoryOffsetSize>();
    rawMetadata.clear();
    rawMetadata.resize(metadataBlockSize);
    loader.dataLoader(metadataBlockSize, inFile, rawMetadata);

    encryption.decrypt(rawMetadata, decryptedMetadata);

    fileCompressedSize -= metadataBlockSize;

    // 读取数据块
    if (!(numReader.readBinaryStandards<Y_flib::FlagType>() == Y_flib::FlagType::Separated))
        throw std::runtime_error("decompressionLoop()-Error:Can't read SEPARATED_FLAG before data block");

    Y_flib::DirectoryOffsetSize blockSize = numReader.readBinaryStandards<Y_flib::DirectoryOffsetSize>();
    rawData.clear();
    rawData.resize(blockSize);
    loader.dataLoader(blockSize, inFile, rawData);

    Y_flib::FileSize readedSize = inFile.gcount();
    if (readedSize != blockSize)
    {
        throw std::runtime_error(
            "decompressionLoop()-Error: Failed to read complete data block. Expected " +
            std::to_string(blockSize) + " bytes, got " + std::to_string(readedSize));
    }

    // 解密并解压
    encryption.decrypt(rawData, decryptedData);

    Y_flib::FileSize remainingBytes = originalSize - totalDecompressedBytes;
    decompressedData.clear();
    compression.decompress(decryptedMetadata, decryptedData, decompressedData, remainingBytes);

    totalDecompressedBytes += decompressedData.size();

    // 写入解压后的数据
    dataExporter.exportDecompressedData(decompressedData);

    fileCompressedSize -= blockSize;
}

void DecompressionLoop::reportProgress(
    const std::filesystem::path &filename,
    Y_flib::FileSize totalDecompressedBytes,
    Y_flib::FileSize originalSize,
    std::chrono::steady_clock::time_point &lastCallbackTime,
    double &lastReportedProgress)
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCallbackTime).count();
    double fileProgress = (originalSize > 0) ? (100.0 * totalDecompressedBytes / originalSize) : 100.0;
    double overallProgress = m_totalFiles > 0 ? (100.0 * (m_processedFiles + fileProgress / 100.0) / m_totalFiles) : fileProgress;

    bool shouldReport = (elapsed >= PROGRESS_CALLBACK_INTERVAL_MS) ||
                        (overallProgress - lastReportedProgress >= 5.0) ||
                        (totalDecompressedBytes >= originalSize);

    if (m_progressCallback && shouldReport)
    {
        m_progressCallback(filename.string(), fileProgress, overallProgress, "Decompressing");
        lastCallbackTime = now;
        lastReportedProgress = overallProgress;
    }
}

void DecompressionLoop::createDirectory(const std::filesystem::path &directoryPath)
{
    try
    {
        if (!directoryPath.parent_path().empty() && !std::filesystem::exists(directoryPath.parent_path()))
        {
            createDirectory(directoryPath.parent_path());
        }

        std::filesystem::create_directory(directoryPath);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("createDirectory()-Error: " + directoryPath.string());
    }
}

bool DecompressionLoop::createFile(const std::filesystem::path &filePath)
{
    try
    {
        if (std::filesystem::exists(filePath))
        {
            std::cerr << "fileIsExist: " << filePath << " ,skipped to next \n";
            return false;
        }

        if (!filePath.parent_path().empty() && !std::filesystem::exists(filePath.parent_path()))
        {
            createDirectory(filePath.parent_path());
        }

        std::ofstream outfile(filePath);
        outfile.close();
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        throw std::runtime_error("createDirectory()-Error: " + filePath.string());
    }
    return true;
}
