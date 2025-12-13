// HeaderLoader.h
#pragma once
#include "FileLibrary.h"
#include "Directory_FileDetails.h"
#include "ToolClasses.h"

#include "DataLoader.h"
#include "DataExporter.h"
#include "Aes.h"
#include "BinaryIO_Loader.h"

class HeaderLoader_Decompression : public BinaryIO_Loader
{
private:
    std::string inPath;

public:
    HeaderLoader_Decompression(const std::string inPath){}
    void headerLoader(const std::string deCompressionFilePath, Aes &aes){

        
    }
};
class HeaderLoader_Compression : public BinaryIO_Loader
{
private:
    Transfer transfer;
    fs::path loadPath;
    DataLoader *dataLoader;

public:
    void headerLoader(const std::string compressionFilePath, const std::vector<std::string> &filePathToScan, Aes &aes);
};