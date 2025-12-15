// HeaderLoader.h
#pragma once
#include "FileLibrary.h"
#include "DataLoader.h"
#include "DataExporter.h"
#include "BinaryIO_Loader.h"
#include "Aes.h"
class HeaderLoader_Decompression
{
private:

    std::string deCompressionFilePath;

public:
    HeaderLoader_Decompression(const std::string deCompressionFilePath)
    {
        this->deCompressionFilePath = deCompressionFilePath;
    }
    void headerLoader(Aes &aes);
};
class HeaderLoader_Compression
{
private:
    Transfer transfer;
    std::string compressionFilePath;

public:
    HeaderLoader_Compression(const std::string compressionFilePath) { this->compressionFilePath = compressionFilePath ;}
    void headerLoader(const std::vector<std::string> &filePathToScan, Aes &aes);
};