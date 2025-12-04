// HeaderLoader.h
#pragma once
#include "../include/FileLibrary.h"
#include "../include/FileDetails.h"
#include "../include/ToolClasses.hpp"

static int countOfD_F = 0;//临时全局变量

class FilePath_Loader
{
private:
    fs::path outPutFilePath;
    fs::path filePathToScan;
};

class BinaryIO_Loader
{
private:
    std::vector<unsigned char> &buffer;
    FileQueue queue;
    std::ifstream &inFile;
    Transfer transfer;

    // 检查 buffer 是否足够读取指定大小
    void loadBySepratedFlag(NumsReader &numsReader, DirectoryOffsetSize_uint &offset, std::vector<std::string> &filePathToScan, FileCount_uint &countOfKidDirectory);

public:
    BinaryIO_Loader(std::vector<unsigned char> &buffer, std::ifstream &inFile)
        : buffer(buffer), inFile(inFile) {}
    void headerLoader(std::vector<std::string> &filePathToScan); // 主逻辑函数

#pragma pack(1) // 禁用填充，紧密读取
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
