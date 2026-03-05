#include "../include/DataExporter.h"

void DataExporter::thisBlockIsDone(Y_flib::DirectoryOffsetSize dataSize)
{
    std::streamoff currentPos = outFile.tellp();
    std::streamoff offsetToFill = currentPos - static_cast<std::streamoff>(dataSize + sizeof(Y_flib::DirectoryOffsetSize));
    outFile.seekp(offsetToFill, std::ios::beg);
    DataWriter dataWriter;
    dataWriter.writeBinaryNums(dataSize, outFile);
    outFile.seekp(0, std::ios::end);
}

void DataExporter::thisFileIsDone(Y_flib::FileSize offsetToFill)
{
    outFile.seekp(offsetToFill, std::ios::beg);
    DataWriter dataWriter;
    dataWriter.writeBinaryNums(processedFileSize, outFile);
    outFile.seekp(0, std::ios::end);
    outFile.flush();  // 强制写入磁盘
    processedFileSize = 0;
}

void DataExporter::exportDataToFile_Compression(const Y_flib::DataBlock &data)
{
    std::ofstream tempFilePtr;

    BinaryIO_Writer processor(tempFilePtr);

    outFile.seekp(0, std::ios::end);
    Y_flib::FileSize dataSize = data.size();
    processor.writeBlankSeparatedStandardForEncryption(outFile);

    outFile.write(reinterpret_cast<const char*>(data.data()), dataSize);
    processedFileSize += dataSize;

    thisBlockIsDone(dataSize);
}
void DataExporter::exportDataToFile_Decompression(const Y_flib::DataBlock &data)
{
    outFile.seekp(0, std::ios::end);
    Y_flib::FileSize dataSize = data.size();

    outFile.write(reinterpret_cast<const char*>(data.data()), dataSize);
    outFile.flush();
    processedFileSize += dataSize;
}