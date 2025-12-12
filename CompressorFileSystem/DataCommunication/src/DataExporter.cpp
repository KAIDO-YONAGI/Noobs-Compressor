#include "../include/DataExporter.h"

void DataExporter::thisBlockIsDone(DirectoryOffsetSize_uint dataSize)
{
    std::streamoff offsetToFill = outFile.tellp() - static_cast<std::streamoff>(dataSize + sizeof(DirectoryOffsetSize_uint));
    outFile.seekp(offsetToFill, std::ios::beg);
    NumsWriter numWriter;
    numWriter.writeBinaryNums(dataSize, outFile);
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

void DataExporter::exportDataToFile_Encryption(const std::vector<char> &data)
{
    if (!outFile)
    {
        throw std::runtime_error("exportDataToFile()-Error:Failed to open outFile");
    }
    BinaryIO_Writter processor(tempFilePtr); // 此处只传入不使用(使用禁止)

    // TODO:此处需要回填偏移量
    outFile.seekp(0, std::ios::end);
    FileSize_uint dataSize = data.size();
    processor.writeBlankSeparatedStandardForEncryption(outFile);

    outFile.write(data.data(), dataSize);
    processedFileSize += dataSize;
    // std::cout << "FileProcessedSize:" << processedFileSize << "\n";

    thisBlockIsDone(dataSize);
}