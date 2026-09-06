#include "../include/DataExporter.h"

namespace Y_flib
{

    void DataExporter::thisFileIsDone(Y_flib::SlotOffset offsetToFill)
    {
        locator.locateFromBegin(outFile, offsetToFill);
        standardWriter.writeBinaryStandards(processedFileSize, outFile); // Backfill processed size
        locator.locateFromEnd(outFile, 0);
        processedFileSize = 0;
    }

    void DataExporter::exportCompressedData(const Y_flib::DataBlock &data)
    {
        Y_flib::BlockLength dataSize = data.size();

        locator.locateFromEnd(outFile, 0);
        // 加密数据块不单独预留 IV；直接写入分隔标志和块长度。
        standardWriter.writeBinaryStandards(Y_flib::FlagType::Separated, outFile);
        standardWriter.writeBinaryStandards(Y_flib::BlockLength(dataSize), outFile);

        StandardsWriter::writeDataBlock(dataSize, outFile, data); // Write block directly to output file
        processedFileSize += dataSize;
    }

    void DataExporter::exportDecompressedData(const Y_flib::DataBlock &data)
    {
        locator.locateFromEnd(outFile, 0);
        Y_flib::FileSize dataSize = data.size();
        StandardsWriter::writeDataBlock(dataSize, outFile, data);
        processedFileSize += dataSize;
    }
} // namespace Y_flib
