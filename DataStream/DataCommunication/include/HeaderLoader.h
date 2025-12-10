// HeaderLoader.h
#pragma once
#include "../include/FileLibrary.h"
#include "../include/Directory_FileDetails.h"
#include "../include/ToolClasses.hpp"
#include "../include/Parser.hpp"

class FilePath_Loader
{
private:
    fs::path outPutFilePath;
    fs::path filePathToScan;
};

class BinaryIO_Loader
{
private:
    std::vector<unsigned char> buffer =
        std::vector<unsigned char>(BUFFER_SIZE + 1024); // 私有buffer,预留1024字节防止溢出
    std::vector<std::string> filePathToScan;            // 构造时初始化，而且只使用一次
    bool isDone = false;                                // 标记是否完成所有目录读取
    Header header;                                      // 私有化存储当前文件头信息

    FileCount_uint countOfKidDirectory=0;  // 当前处理中或退出时目录下子目录或文件数量
    DirectoryOffsetSize_uint offset=0;     // 当前剩余字节数
    DirectoryOffsetSize_uint tempOffset=0; // 当前处理块的大小（偏移）

    std::ifstream inFile;

    Transfer transfer;
    Parser *parserForLoader; // 私有化工具类实例，避免重复构造与析构

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
    Directory_FileQueue fileQueue;      // 文件队列
    Directory_FileQueue directoryQueue; // 目录队列

    BinaryIO_Loader(std::string inPath, std::vector<std::string> filePathToScan = {})
    {
        fs::path loadPath = transfer.transPath(inPath);
        std::ifstream inFile(loadPath, std::ios::binary);
        this->inFile = std::move(inFile);
        this->filePathToScan = filePathToScan;
        this->parserForLoader = new Parser(buffer, directoryQueue, fileQueue, header, offset, tempOffset);
    }
    ~BinaryIO_Loader()
    {
        done();
        delete parserForLoader;
    }
    bool loaderLoopIsDone()
    {
        return isDone;
    }

    void headerLoader(); // 主逻辑函数
};
