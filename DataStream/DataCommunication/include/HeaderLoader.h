// HeaderLoader.h
#pragma once
#include "../include/FileLibrary.h"
#include "../include/Directory_FileDetails.h"
#include "../include/ToolClasses.hpp"
#include "../include/Parser.hpp"
#include "../include/DataLoader.h"
// static int countOfD_F = 0;//临时全局变量

class FilePath_Loader
{
private:
    fs::path outPutFilePath;
    fs::path filePathToScan;
};

class BinaryIO_Loader
{
private:
    std::vector<unsigned char> buffer = std::vector<unsigned char>(BUFFER_SIZE + 1024);
    std::vector<std::string> filePathToScan;
    bool isDone = false;
    Header header;

    FileCount_uint countOfKidDirectory;
    DirectoryOffsetSize_uint offset;
    DirectoryOffsetSize_uint tempOffset ;

    std::ifstream inFile;
    Transfer transfer;
    Parser *parserForLoader;
    // 检查 buffer 是否足够读取指定大小
    void loadBySepratedFlag(NumsReader &numsReader, FileCount_uint &countOfKidDirectory);
    void done()
    {
        if (inFile.is_open())
        {
            inFile.close();
        }
        isDone = true;
    }

public:
    Directory_FileQueue fileQueue;
    Directory_FileQueue directoryQueue;

    BinaryIO_Loader(std::string inPath, std::vector<std::string> filePathToScan = {})
    {
        fs::path loadPath = transfer.transPath(inPath);
        std::ifstream inFile(loadPath, std::ios::binary);
        this->inFile = std::move(inFile);
        this->filePathToScan = filePathToScan;
        this->parserForLoader = new Parser(buffer, directoryQueue, fileQueue, header, offset,tempOffset);
    }
    ~BinaryIO_Loader()
    {
        if (inFile.is_open())
        {
            inFile.close();
        }
    }

    void headerLoader(); // 主逻辑函数
};
