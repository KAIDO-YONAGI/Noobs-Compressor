// HeaderLoader.h
#pragma once
#include "../include/FileLibrary.h"
#include "../include/Directory_FileDetails.h"
#include "../include/ToolClasses.hpp"

#include "../include/DataLoader.h"
// static int countOfD_F = 0;//临时全局变量

class FilePath_Loader
{
private:
    fs::path outPutFilePath;
    fs::path filePathToScan;
};
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

class BinaryIO_Loader
{
private:
    std::vector<unsigned char> &buffer;
    bool isDone=false;
    Header header;

    Directory_FileQueue directoryQueue;
    Directory_FileQueue fileQueue;
    FileCount_uint countOfKidDirectory;
    DirectoryOffsetSize_uint offset;

    std::ifstream inFile;
    Transfer transfer;

    // 检查 buffer 是否足够读取指定大小
    void loadBySepratedFlag(NumsReader &numsReader, DirectoryOffsetSize_uint &offset, std::vector<std::string> &filePathToScan, FileCount_uint &countOfKidDirectory);
    void done(){
        if (inFile.is_open())
        {
            inFile.close();
        }
        isDone=true;
    }
    
public:
    BinaryIO_Loader(std::vector<unsigned char> &buffer, std::string inPath)
        : buffer(buffer)
    {
        fs::path loadPath = transfer.transPath(inPath);
        std::ifstream inFile(loadPath, std::ios::binary);
        this->inFile = std::move(inFile);
    }
    ~BinaryIO_Loader()
    {
        if (inFile.is_open())
        {
            inFile.close();
        }
    }

    void headerLoader(std::vector<std::string> &filePathToScan); // 主逻辑函数
};
