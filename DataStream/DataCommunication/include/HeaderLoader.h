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
private:
    std::vector<char> &buffer;
    std::ifstream &inFile;

    template <typename T>
    void fileNameSizeParser(
        T &fileNameSize,
        std::string &fileName,
        DirectoryOffsetSize_uint &bufferPtr)
    {
        memcpy(&fileNameSize, buffer.data() + bufferPtr, sizeof(T));
        bufferPtr += sizeof(T);
        memcpy(&fileName, buffer.data() + bufferPtr, fileNameSize);
        bufferPtr += fileName.size();
    }
    template <typename T>
    T numsParser(T &num, DirectoryOffsetSize_uint &bufferPtr)
    {
        memcpy(&num, buffer.data() + bufferPtr, sizeof(T));
        bufferPtr += sizeof(T);
    }

public:
    BinaryIO_Loader(std::vector<char> &buffer, std::ifstream &inFile) : buffer(buffer), inFile(inFile) {};
    void headerLoader();//Ö÷Âß¼­º¯Êý

#pragma pack(1) // ½ûÓÃÌî³ä£¬½ôÃÜ¶ÁÈ¡
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

#endif
