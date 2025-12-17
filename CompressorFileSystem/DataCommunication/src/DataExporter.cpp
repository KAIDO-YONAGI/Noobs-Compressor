#include "../include/DataExporter.h"

void DataExporter::thisBlockIsDone(DirectoryOffsetSize_uint dataSize)
{
    std::cout << "[EXPORT] thisBlockIsDone called with dataSize=" << dataSize << " bytes\n";
    std::streamoff currentPos = outFile.tellp();
    std::streamoff offsetToFill = currentPos - static_cast<std::streamoff>(dataSize + sizeof(DirectoryOffsetSize_uint));
    std::cout << "[EXPORT] Current file position: " << currentPos << ", backfilling size at offset: " << offsetToFill << "\n";
    outFile.seekp(offsetToFill, std::ios::beg);
    NumsWriter numWriter;
    numWriter.writeBinaryNums(dataSize, outFile);
    outFile.flush();  // 强制写入磁盘
    outFile.seekp(0, std::ios::end);
    std::cout << "[EXPORT] Backfilled block size=" << dataSize << " bytes and flushed to disk\n";
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

    BinaryIO_Writter processor(tempFilePtr); // �˴�ֻ���벻ʹ��(ʹ�ý�ֹ)

    outFile.seekp(0, std::ios::end);
    FileSize_uint dataSize = data.size();
    processor.writeBlankSeparatedStandardForEncryption(outFile);

    outFile.write(reinterpret_cast<const char*>(data.data()), dataSize);
    processedFileSize += dataSize;
    // std::cout << "FileProcessedSize:" << processedFileSize << "\n";

    thisBlockIsDone(dataSize);
}
void DataExporter::exportDataToFile_Decompression(const DataBlock &data)
{
    outFile.seekp(0, std::ios::end);
    FileSize_uint dataSize = data.size();

    outFile.write(reinterpret_cast<const char*>(data.data()), dataSize);
    processedFileSize += dataSize;
    // std::cout << "FileProcessedSize:" << processedFileSize << "\n";

}