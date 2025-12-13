// HeaderLoader.h
#pragma once
#include "FileLibrary.h"
#include "Directory_FileDetails.h"
#include "ToolClasses.h"

#include "DataLoader.h"
#include "DataExporter.h"
#include "Aes.h"
#include "BinaryIO.h"

class HeaderLoader_Decompression : public BinaryIO_Loader
{
private:
    std::string inPath;

public:
    Directory_FileQueue directoryQueue_ready;
    HeaderLoader_Decompression(const std::string inPath)
        : BinaryIO_Loader(inPath), inPath(inPath) {}
    void headerLoader(const std::string deCompressionFilePath, Aes &aes){

        
    }
};
class HeaderLoader_Compression
{
private:
    Transfer transfer;
    fs::path loadPath;
    DataLoader *dataLoader;

public:
    void headerLoader(const std::string compressionFilePath, const std::vector<std::string> &filePathToScan, Aes &aes);
};