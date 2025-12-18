#include "../include/DataExporter.h"

void DataExporter::thisBlockIsDone(DirectoryOffsetSize_uint dataSize)
{
    std::streamoff currentPos = outFile.tellp();
    std::streamoff offsetToFill = currentPos - static_cast<std::streamoff>(dataSize + sizeof(DirectoryOffsetSize_uint));
    outFile.seekp(offsetToFill, std::ios::beg);
    NumsWriter numWriter;
    numWriter.writeBinaryNums(dataSize, outFile);
    outFile.flush();  // 强制写入磁盘
    outFile.seekp(0, std::ios::end);
}

void DataExporter::thisFileIsDone(FileSize_uint offsetToFill)
{
    outFile.seekp(offsetToFill, std::ios::beg);
    NumsWriter numWriter;
    numWriter.writeBinaryNums(processedFileSize, outFile);
    outFile.seekp(0, std::ios::end);

    processedFileSize = 0;
}

void DataExporter::exportDataToFile_Compression(const DataBlock &data)
{
    std::ofstream tempFilePtr;

    BinaryIO_Writter processor(tempFilePtr);

    outFile.seekp(0, std::ios::end);
    FileSize_uint dataSize = data.size();
    processor.writeBlankSeparatedStandardForEncryption(outFile);

    outFile.write(reinterpret_cast<const char*>(data.data()), dataSize);
    processedFileSize += dataSize;

    thisBlockIsDone(dataSize);
}
void DataExporter::exportDataToFile_Decompression(const DataBlock &data)
{
    outFile.seekp(0, std::ios::end);
    FileSize_uint dataSize = data.size();

    outFile.write(reinterpret_cast<const char*>(data.data()), dataSize);
    processedFileSize += dataSize;
}