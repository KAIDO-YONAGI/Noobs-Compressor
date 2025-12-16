#pragma once

#include "FileLibrary.h"
#include "ToolClasses.h"
#include "Directory_FileDetails.h"
#include "Directory_FileParser.h"
#include "My_Aes.h"
#include <queue>
class BinaryIO_Loader
{
private:
    bool blockIsDone = false;
    bool allDone = false;   // 标记是否完成所有目录读取
    bool FirstReady = true; // 标记当前是否是目录就绪队列第一个元素

    FileCount_uint countOfKidDirectory = 0;  // 当前处理中或退出时目录下子目录或文件数量
    DirectoryOffsetSize_uint offset = 0;     // 当前剩余字节数
    DirectoryOffsetSize_uint tempOffset = 0; // 当前处理块的大小（偏移）

    fs::path loadPath;
    fs::path parentPath;
    std::ifstream inFile;
    std::fstream fstreamForRefill;
    std::vector<std::string> filePathToScan; // 构造时初始化，而且只使用一次

    Transfer transfer;
    Header header;                         // 私有化存储当前文件头信息
    std::unique_ptr<Directory_FileParser>parserForLoader; // 私有化工具类实例，避免重复构造与析构
    DataBlock buffer =
        DataBlock(BUFFER_SIZE + 1024); // 私有buffer,预留1024字节防止溢出

    void requestDone();

    void allLoopDone();

    void loadBySepratedFlag(NumsReader &numsReader, FileCount_uint &countOfKidDirectory, Aes &aes);

public:
    void headerLoaderIterator(Aes &aes); // 主逻辑函数

    // 压缩时队列
    Directory_FileQueue fileQueue;                            // 文件队列
    Directory_FileQueue directoryQueue;                       // 目录队列
    std::vector<std::array<DirectoryOffsetSize_uint, 2>> pos; // 目录数据块位置数组 1 为起点，2为大小

    // 解压时队列
    std::queue<fs::path> directoryQueue_ready; // 目录恢复就绪队列，文件复原需要在目录恢复后操作

    BinaryIO_Loader() {};
    BinaryIO_Loader(const std::string inPath, std::vector<std::string> filePathToScan, fs::path parentPath)
    {
        this->loadPath = transfer.transPath(inPath);
        std::ifstream inFile(loadPath, std::ios::binary);
        std::fstream fstreamForRefill(loadPath, std::ios::binary | std::ios::in | std::ios::out);
        if (!inFile)
            throw std::runtime_error("BinaryIO_Loader()-Error:Failed to open inFile" + inPath);
        if (!fstreamForRefill)
            throw std::runtime_error("BinaryIO_Loader()-Error:Failed to open fstreamForRefill" + inPath);

        this->inFile = std::move(inFile);
        this->fstreamForRefill = std::move(fstreamForRefill);
        this->filePathToScan = filePathToScan;
        this->parserForLoader = std::make_unique<Directory_FileParser>
            (buffer, directoryQueue, fileQueue, header, offset, tempOffset, this->filePathToScan);
        this->parentPath = parentPath;
    }

    ~BinaryIO_Loader() { allLoopDone(); }

    DirectoryOffsetSize_uint getDirectoryOffset() { return header.directoryOffset; }

    bool allLoopIsDone() { return allDone; }
    bool loaderRequestIsDone() { return blockIsDone; }
    std::ifstream &getInFile() { return inFile; }
    void restartLoader();
    void encryptHeaderBlock(Aes &aes);
};
