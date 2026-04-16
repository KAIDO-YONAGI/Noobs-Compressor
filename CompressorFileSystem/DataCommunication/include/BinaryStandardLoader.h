#pragma once

#include "FileLibrary.h"
#include "ToolClasses.h"
#include "EntryDetails.h"
#include "EntryParser.h"
#include "My_Aes.h"
#include <queue>
#include <algorithm>
#include <cstring>
/*
 BinaryStandardLoader - 二进制目录块读取与解析器

   功能:
   从.sy文件读取加密的目录块
   调用DEntryarser解析二进制目录结构
   管理文件队列、目录队列用于解压流程
   支持分块读取和AES解密

   公共接口:
   headerLoaderIterator(): 主循环函数，逐块读取目录数据
   getDirectoryOffset(): 获取目录块偏移量
   allLoopIsDone(): 检查是否完成所有读取
   restartLoader(): 重新初始化读取状态
   encryptHeaderBlock(): 加密并回填目录块
*/
class BinaryStandardLoader
{
private:
    bool isReadHeader = false;
    bool blockIsDone = false;
    bool allDone = false;   // 标记是否完成所有目录读取
    bool FirstReady = true; // 标记当前是否是目录就绪队列第一个元素

    Y_flib::FileCount countOfChildDirectory = 0; // 当前处理中或退出时目录下子目录或文件数量
    Y_flib::FileSize offset = 0;                 // 当前剩余字节数
    Y_flib::DirectoryOffsetSize tempOffset = 0;  // 当前处理块的大小（偏移）

    std::filesystem::path loadPath;
    std::filesystem::path parentPath;
    std::ifstream inFile;
    std::fstream fstreamForRefill;
    std::vector<std::string> filePathToScan; // 构造时初始化，而且只使用一次

    PathTransfer transfer;
    Y_flib::Header header;                                // 私有化存储当前文件头信息
    std::unique_ptr<EntryParser> parserForLoader; // 私有化工具类实例，避免重复构造与析构
    Y_flib::DataBlock buffer =
        Y_flib::DataBlock(Y_flib::Constants::BUFFER_SIZE + 1024); // 私有buffer,预留1024字节防止溢出

    void setRequestDone();                                                                                                 // 标记块读取完成
    void setAllLoopDone();                                                                                                 // 标记所有循环完成并清理资源
    void loadEntryBlock(StandardsReader &standardsReader, Y_flib::FileCount &countOfChildDirectory, Aes &aes); // 读取单个数据块、解密、解析
    void loadHeaderStandard(std::ifstream &inFile, Y_flib::Header &header, Y_flib::DataBlock &buffer);
    void loadSeparatedStandard(Y_flib::FlagType &flag, StandardsReader &standardsReader, Y_flib::IvSize &ivNum);

public:
    void headerLoaderIterator(Aes &aes); // 主读取循环：逐块读取、解密、解析目录结构

    // 压缩时队列
    EntryQueue fileQueue;                                                  // 文件队列
    EntryQueue entryQueue;                                                 // 目录队列
    std::vector<std::array<Y_flib::DirectoryOffsetSize, 2>> blockPosition; // 目录数据块位置数组 1 为起点，2为大小

    // 解压时队列
    std::queue<std::filesystem::path> directoryQueue_ready; // 目录恢复就绪队列，文件复原需要在目录恢复后操作

    BinaryStandardLoader() {};
    BinaryStandardLoader(const std::string inPath, std::vector<std::string> filePathToScan, std::filesystem::path parentPath)
    {
        this->loadPath = transfer.transPath(inPath);

        this->inFile = std::ifstream(loadPath, std::ios::binary);

        this->fstreamForRefill = std::fstream(loadPath, std::ios::binary | std::ios::in | std::ios::out);

        if (!inFile)
            throw std::runtime_error("BinaryStandardLoader()-Error:Failed to open inFile" + inPath);
        if (!fstreamForRefill)
            throw std::runtime_error("BinaryStandardLoader()-Error:Failed to open fstreamForRefill" + inPath);

        this->filePathToScan = filePathToScan;
        this->parserForLoader = std::make_unique<EntryParser>(buffer, entryQueue, fileQueue, header, offset, tempOffset, this->filePathToScan);
        this->parentPath = parentPath;
    }

    ~BinaryStandardLoader() {
        setAllLoopDone();
        // 析构时关闭文件流
        if (inFile.is_open()) {
            inFile.close();
        }
        if (fstreamForRefill.is_open()) {
            fstreamForRefill.close();
        }
    }

    Y_flib::DirectoryOffsetSize getDirectoryOffset() { return header.directoryOffset; } // 获取目录块偏移量

    bool allLoopIsDone() { return allDone; } // 检查所有读取是否完成

    bool loaderRequestIsDone() { return blockIsDone; } // 检查当前块读取是否完成

    std::ifstream &getInFile() { return inFile; } // 获取输入文件对象

    void restartLoader(); // 重新打开文件并定位到当前偏移

    void encryptHeaderBlock(Aes &aes); // 在压缩流程中读取完目录信息就直接加密并回填目录块到文件（读信息->填目录->读目录->加密回填 其中的后两步）
};
