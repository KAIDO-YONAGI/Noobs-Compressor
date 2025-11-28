// HeaderLoader.h
#ifndef HEADERLOADER_H
#define HEADERLOADER_H

#include "../include/FileLibrary.h"
#include "../include/FileDetails.h"
#include "../include/ToolClasses.hpp"

class FilePath_Loader
{
private:
    fs::path outPutFilePath;
    fs::path filePathToScan;
};
class BinaryIO_Loader
{
public:
#pragma pack(1) // Ω˚”√ÃÓ≥‰£¨ΩÙ√‹∂¡»°
    struct Header
    {
        SizeOfMagicNum_uint magicNum = 0;
        CompressStrategy_uint strategy = 0;
        CompressorVersion_uint version = 0;
        HeaderOffsetSize_uint headerOffset = 0;
        DirectoryOffsetSize_uint directoryOffset = 0;
        Header(
            SizeOfMagicNum_uint magicNum,
            CompressStrategy_uint strategy,
            CompressorVersion_uint version,
            HeaderOffsetSize_uint headerOffset,
            DirectoryOffsetSize_uint directoryOffset)
            : magicNum(magicNum), strategy(strategy), version(version),
              headerOffset(headerOffset), directoryOffset(directoryOffset) {};
    };

    struct DirectoryData
    {
    };
    struct FileData
    {
    };
    struct SeparatedData
    {
    };
};
template <typename T>
T read_binary_le(std::ifstream &file)
{
    T value;
    file.read(reinterpret_cast<char *>(&value), sizeof(T));
    return value;
}

#endif
