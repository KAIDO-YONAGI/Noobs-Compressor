#include "../include/DataExporter.h"

void DataExporter::thisBlockIsDone(Y_flib::DirectoryOffsetSize dataSize)
{
    StandardsWriter standardWriter;
    std::streamoff currentPos = outFile.tellp();
    std::streamoff offsetToFill = currentPos - static_cast<std::streamoff>(dataSize + sizeof(Y_flib::DirectoryOffsetSize));
    locator.locateFromBegin(outFile, offsetToFill);
    standardWriter.writeBinaryStandards(dataSize, outFile);
    locator.locateFromEnd(outFile, 0);
}

void DataExporter::thisFileIsDone(Y_flib::FileSize offsetToFill)
{
    StandardsWriter standardWriter;
    locator.locateFromBegin(outFile, offsetToFill);
    standardWriter.writeBinaryStandards(processedFileSize, outFile);
    locator.locateFromEnd(outFile, 0);
    processedFileSize = 0;
}

void DataExporter::exportDataToFileCompression(const Y_flib::DataBlock &data)
{
    std::ofstream tempFilePtr;

    BinaryStandardWriter processor(tempFilePtr);
    Y_flib::FileSize dataSize = data.size();

    locator.locateFromEnd(outFile, 0);
    processor.writeBlankSeparatedStandardForEncryption(outFile);

    outFile.write(reinterpret_cast<const char*>(data.data()), dataSize);
    processedFileSize += dataSize;

    thisBlockIsDone(dataSize);
}
void DataExporter::exportDataToFileDecompression(const Y_flib::DataBlock &data)
{
    locator.locateFromEnd(outFile, 0);
    Y_flib::FileSize dataSize = data.size();

    outFile.write(reinterpret_cast<const char*>(data.data()), dataSize);
    processedFileSize += dataSize;
}