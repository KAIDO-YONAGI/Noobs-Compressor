// HeaderLoader.h
#pragma once
#include "FileLibrary.h"
#include "Directory_FileDetails.h"
#include "ToolClasses.h"
#include "Directory_FileParser.h"

#include "../CompressorFileSystem/DataCommunication/include/DataLoader.h"
#include "../CompressorFileSystem/DataCommunication/include/DataExporter.h"

#include "../EncryptionModules/Aes/include/Aes.h"

class BinaryIO_Loader_Compression
{
private:
    DataBlock buffer =
        DataBlock(BUFFER_SIZE + 1024);       // 私有buffer,预留1024字节防止溢出
    std::vector<std::string> filePathToScan; // 构造时初始化，而且只使用一次
    bool blockIsDone = false;
    bool allDone = false; // 标记是否完成所有目录读取
    Header header;        // 私有化存储当前文件头信息

    FileCount_uint countOfKidDirectory = 0;  // 当前处理中或退出时目录下子目录或文件数量
    DirectoryOffsetSize_uint offset = 0;     // 当前剩余字节数
    DirectoryOffsetSize_uint tempOffset = 0; // 当前处理块的大小（偏移）

    fs::path loadPath;
    std::ifstream inFile;

    Transfer transfer;
    Directory_FileParser *parserForLoader; // 私有化工具类实例，避免重复构造与析构
    void requesetDone()
    {
        if (inFile.is_open())
        {
            inFile.close();
        }
        blockIsDone = true;
    }
    void allLoopDone()
    {
        if (inFile.is_open())
        {
            inFile.close();
        }
        allDone = true;
    }
    void loadBySepratedFlag(NumsReader &numsReader, FileCount_uint &countOfKidDirectory);

public:
    Directory_FileQueue fileQueue;      // 文件队列
    Directory_FileQueue directoryQueue; // 目录队列

    void headerLoader(); // 主逻辑函数

    BinaryIO_Loader_Compression() {};
    BinaryIO_Loader_Compression(const std::string inPath, std::vector<std::string> filePathToScan = {})
    {
        this->loadPath = transfer.transPath(inPath);
        std::ifstream inFile(loadPath, std::ios::binary);
        if (!inFile)
            throw std::runtime_error("BinaryIO_Loader_Compression()-Error:Failed to open inFile" + inPath);

        this->inFile = std::move(inFile);
        this->filePathToScan = filePathToScan;
        this->parserForLoader = new Directory_FileParser(buffer, directoryQueue, fileQueue, header, offset, tempOffset);
    }

    ~BinaryIO_Loader_Compression()
    {
        allLoopDone();
        delete parserForLoader;
    }
    void restartLoader()
    {
        if (!allLoopIsDone())
        {
            std::ifstream newInFile(loadPath, std::ios::binary);
            if (!newInFile)
                throw std::runtime_error("restartLoader()-Error:Failed to open inFile");

            size_t offsetToRestart = header.directoryOffset - offset;

            newInFile.seekg(offsetToRestart, std::ios::beg);
            this->inFile = std::move(newInFile);
            blockIsDone = false;
        }
        else
            return;
    }
    bool allLoopIsDone()
    {
        return allDone;
    }
    bool loaderRequestIsDone()
    {
        return blockIsDone;
    }
};

class BinaryIO_Loader_Decompression : public BinaryIO_Loader_Compression
{
private:
    std::string inPath;

public:
    Directory_FileQueue directoryQueue_ready;
    BinaryIO_Loader_Decompression(const std::string inPath) : inPath(inPath) {}
};
class HeaderLoader_Compression
{
private:
    Transfer transfer;
    fs::path loadPath;
    Locator locator;
    DataLoader *dataLoader;

public:
    void headerLoader(const std::string compressionFilePath, const std::vector<std::string> &filePathToScan, Aes &aes);
};