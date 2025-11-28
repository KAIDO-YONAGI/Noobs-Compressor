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
        SizeOfMagicNum_uint magicNum_1 = 0;
        CompressStrategy_uint strategy = 0;
        CompressorVersion_uint version = 0;
        HeaderOffsetSize_uint headerOffset = 0;
        DirectoryOffsetSize_uint directoryOffset = 0;
        SizeOfMagicNum_uint magicNum_2 = 0;
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
