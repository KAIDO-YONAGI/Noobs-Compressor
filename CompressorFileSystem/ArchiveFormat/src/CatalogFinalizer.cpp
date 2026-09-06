#include "../include/CatalogFinalizer.h"
#include "../include/StrategyFactory.h"
#include "FileSystemUtils.h"

namespace Y_flib
{
    CatalogFinalizer::CatalogFinalizer(const std::filesystem::path &archivePath)
    {
        // 归档文件自身也可能位于深层目录，打开流前统一转换实际 I/O 路径
        archiveFile.open(
            FileSystemUtils::pathForIo(archivePath),
            std::ios::binary | std::ios::in | std::ios::out);
        if (!archiveFile)
        {
            throw std::runtime_error("CatalogFinalizer()-Error:Failed to open archive: " +
                                     EncodingUtils::pathToUtf8(archivePath));
        }
    }

    void CatalogFinalizer::finalize(const std::vector<SizeFillEntry> &sizeEntries,
                                    const std::vector<BlockSpan> &directorySpans,
                                    Y_flib::IEncryption &encryption,
                                    Y_flib::CompressionMode mode)
    {
        backfillFileSizes(sizeEntries);
        encryptDirectoryBlocks(directorySpans, encryption, mode);
    }

    void CatalogFinalizer::backfillFileSizes(const std::vector<SizeFillEntry> &sizeEntries)
    {
        for (const SizeFillEntry &entry : sizeEntries)
        {
            locator.locateFromBegin(archiveFile, entry.slotOffset);
            standardWriter.writeBinaryStandards(entry.processedSize, archiveFile);
        }
        if (!sizeEntries.empty())
            locator.locateFromEnd(archiveFile, 0); // 写指针归位文件尾
    }

    void CatalogFinalizer::encryptDirectoryBlocks(const std::vector<BlockSpan> &directorySpans,
                                                  Y_flib::IEncryption &encryption,
                                                  Y_flib::CompressionMode mode)
    {
        // 非加密模式下，目录块保持明文，无需加密回填
        if (!Y_flib::StrategyFactory::hasEncryption(mode))
            return;

        Y_flib::DataBlock inBlock;
        Y_flib::DataBlock encryptedBlock;

        for (const BlockSpan &span : directorySpans)
        {
            const Y_flib::SlotOffset startPos = span.startPos;
            const Y_flib::BlockLength blockSize = span.size;

            inBlock.resize(blockSize);
            encryptedBlock.resize(blockSize + Y_flib::Constants::IV_BYTES);

            locator.locateFromBegin(archiveFile, startPos); // 定位到数据块起始位置

            StandardsReader::readDataBlock(blockSize, archiveFile, inBlock); // 读取数据块到buffer

            encryption.encrypt(inBlock, encryptedBlock);
            locator.locateFromBegin(archiveFile, span.ivSlotPos()); // 定位到数据块前的 IV 预留空间，准备回写加密数据

            StandardsWriter::writeDataBlock(blockSize + Y_flib::Constants::IV_BYTES, archiveFile, encryptedBlock); // 回写加密数据

            inBlock.clear();
            encryptedBlock.clear();
        }
    }
} // namespace Y_flib
