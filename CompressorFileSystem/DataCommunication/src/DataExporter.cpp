#include "../include/DataExporter.h"

void DataExporter::thisBlockIsDone(Y_flib::DirectoryOffsetSize dataSize)
{
    std::streamoff currentPos = outFile.tellp();
    std::streamoff offsetToFill = currentPos - static_cast<std::streamoff>(dataSize + sizeof(Y_flib::DirectoryOffsetSize));
    locator.locateFromBegin(outFile, offsetToFill);
    standardWriter.writeBinaryStandards(dataSize, outFile);
    locator.locateFromEnd(outFile, 0);
}

void DataExporter::thisFileIsDone(Y_flib::FileSize offsetToFill)
{
    locator.locateFromBegin(outFile, offsetToFill);
    standardWriter.writeBinaryStandards(processedFileSize, outFile);//回填处理后大小
    locator.locateFromEnd(outFile, 0);
    processedFileSize = 0;
}

void DataExporter::exportCompressedData(const Y_flib::DataBlock &data)
{
    Y_flib::FileSize dataSize = data.size();

    std::ofstream blank;
    BinaryStandardWriter binaryStandardWriter(blank);
    locator.locateFromEnd(outFile, 0);
    binaryStandardWriter.writeBlankSeparatedStandardForEncryption(outFile);

    StandardsWriter::writeDataBlock(dataSize, outFile, data); // 直接写入数据块到输出文件
    processedFileSize += dataSize;

    thisBlockIsDone(dataSize);
}
void DataExporter::exportDecompressedData(const Y_flib::DataBlock &data)
{
    locator.locateFromEnd(outFile, 0);
    Y_flib::FileSize dataSize = data.size();
    StandardsWriter::writeDataBlock(dataSize, outFile, data);
    processedFileSize += dataSize;
}