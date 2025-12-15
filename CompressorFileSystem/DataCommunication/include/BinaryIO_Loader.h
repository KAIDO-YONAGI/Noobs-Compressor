#pragma once

#include "FileLibrary.h"
#include "ToolClasses.h"
#include "Directory_FileDetails.h"
#include "Directory_FileParser.h"
#include "Aes.h"
#include <queue>
class BinaryIO_Loader
{
private:
    DataBlock buffer =
        DataBlock(BUFFER_SIZE + 1024);       // 私有buffer,预留1024字节防止溢出
    std::vector<std::string> filePathToScan; // 构造时初始化，而且只使用一次
    bool blockIsDone = false;
    bool allDone = false;   // 标记是否完成所有目录读取
    bool FirstReady = true; // 标记当前是否是目录就绪队列第一个元素
    Header header;          // 私有化存储当前文件头信息

    FileCount_uint countOfKidDirectory = 0;  // 当前处理中或退出时目录下子目录或文件数量
    DirectoryOffsetSize_uint offset = 0;     // 当前剩余字节数
    DirectoryOffsetSize_uint tempOffset = 0; // 当前处理块的大小（偏移）

    fs::path loadPath;
    std::ifstream inFile;
    std::fstream fstreamForRefill;

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
    void loadBySepratedFlag(NumsReader &numsReader, FileCount_uint &countOfKidDirectory, Aes &aes);

public:
    // 压缩时队列
    Directory_FileQueue fileQueue;                            // 文件队列
    Directory_FileQueue directoryQueue;                       // 目录队列
    std::vector<std::array<DirectoryOffsetSize_uint, 2>> pos; // 目录数据块位置数组 1 为起点，2为大小

    // 解压时队列
    std::queue<fs::path> directoryQueue_ready; // 目录恢复就绪队列，文件复原需要在目录恢复后操作

    void headerLoaderIterator(Aes &aes); // 主逻辑函数

    BinaryIO_Loader() {};
    BinaryIO_Loader(const std::string inPath, std::vector<std::string> filePathToScan)
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
        this->parserForLoader =
            new Directory_FileParser(buffer, directoryQueue, fileQueue, header, offset, tempOffset, this->filePathToScan);
    }

    ~BinaryIO_Loader()
    {
        allLoopDone();
    }
    bool allLoopIsDone()
    {
        return allDone;
    }
    bool loaderRequestIsDone()
    {
        return blockIsDone;
    }
    std::ifstream &getInFile()
    {
        return inFile;
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
    void encryptHeaderBlock(Aes &aes)
    {
        DataBlock inBlock;
        DataBlock encryptedBlock;
        DirectoryOffsetSize_uint startPos = 0, blockSize = 0;

        for (auto blockPos : pos)
        {
            startPos = blockPos[0];
            blockSize = blockPos[1];

            inBlock.resize(blockSize);
            encryptedBlock.resize(blockSize + sizeof(IvSize_uint));

            fstreamForRefill.seekp(startPos, std::ios::beg);
            fstreamForRefill.read(reinterpret_cast<char *>(inBlock.data()), blockSize);

            aes.doAes(1, inBlock, encryptedBlock);
            fstreamForRefill.seekp(startPos - sizeof(IvSize_uint));
            fstreamForRefill.write(reinterpret_cast<const char *>(encryptedBlock.data()), blockSize + sizeof(IvSize_uint));

            inBlock.clear();
            encryptedBlock.clear();
        }
        std::cout << "encryptHeaderBlock-Done" << "\n";
    }
};
