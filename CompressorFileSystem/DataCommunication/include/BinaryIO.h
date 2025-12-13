#pragma once

#include "FileLibrary.h"
#include "ToolClasses.h"
#include "Directory_FileDetails.h"
#include "Directory_FileParser.h"
class BinaryIO_Writter // 接触二进制文件及其处理的相关IO的函数的封装
{
    /*
    binaryIO_Reader()主函数。扫描指定路径（单层）的文件及其子文件夹的函数
    writeStorageStandard()分发当前处理的路径上的目录/文件到：目录标准写入/文件标准写入
    writeDirectoryStandard()目录标准写入
    writeFileStandard()文件标准写入
    writeLogicalRoot()写入逻辑根节点，用于处理多文件（目录）任务，具体情况可见调试代码
    writeRoot()写入根节点（因为filesystem自动忽略了根节点）
    */
private:
    Transfer transfer;
    NumsWriter numWriter;
    Locator locator;
    std::ofstream &outFile;

    void writeDirectoryStandard(Directory_FileDetails &details, FileCount_uint count, DirectoryOffsetSize_uint &tempOffset);
    void writeFileStandard(Directory_FileDetails &details, DirectoryOffsetSize_uint &tempOffset);
    void writeSeparatedStandard(DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint offset);
    void writeStorageStandard(Directory_FileDetails &details, Directory_FIleQueueInterface &directoryQueue, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset);
    void writeSymbolLinkStandard(Directory_FileDetails &details, DirectoryOffsetSize_uint &tempOffset);
    FileCount_uint countFilesInDirectory(const fs::path &filePathToScan);
    FileSize_uint getFileSize(const fs::path &filePathToScan);

public:
    explicit BinaryIO_Writter(std::ofstream &outFile) : outFile(outFile) {};
    void writeLogicalRoot(const std::string &logicalRoot, const FileCount_uint count, DirectoryOffsetSize_uint &tempOffset);
    void writeRoot(FilePath &file, const  std::vector<std::string> &filePathToScan, DirectoryOffsetSize_uint &tempOffset);
    void writeBlankSeparatedStandard();
    void writeBlankSeparatedStandardForEncryption(std::fstream &File);
    void binaryIO_Reader(FilePath &file, Directory_FIleQueueInterface &directoryQueue, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset);
};

class BinaryIO_Loader
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

    BinaryIO_Loader() {};
    BinaryIO_Loader(const std::string inPath, std::vector<std::string> filePathToScan = {})
    {
        this->loadPath = transfer.transPath(inPath);
        std::ifstream inFile(loadPath, std::ios::binary);
        if (!inFile)
            throw std::runtime_error("BinaryIO_Loader()-Error:Failed to open inFile" + inPath);

        this->inFile = std::move(inFile);
        this->filePathToScan = filePathToScan;
        this->parserForLoader = new Directory_FileParser(buffer, directoryQueue, fileQueue, header, offset, tempOffset);
    }

    ~BinaryIO_Loader()
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
    std::ifstream &getInFile(){
        return inFile;
    }
};
