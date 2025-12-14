// Directory_FileProcessor.h
#pragma once

#include "FileLibrary.h"
#include "Directory_FileDetails.h"
#include "ToolClasses.h"
#include "BinaryIO_Writer.h"
class Directory_FileProcessor
{
    /*
    scanFlow()按BFS（层序遍历）扫描指定目录下所有文件的函数。包含了队列逻辑，用于处理binaryIO_Reader()的循环扫描到的目录
    directory_fileProcessor()主函数。参数const const std::vector<std::string> &filePathToScan，用于处理多文件（目录）任务
    */
private:
    Transfer transfer;
    std::ofstream &outFile;
    Directory_FIleQueueInterface directoryQueue;
    FilePath file; // 创建各个工具类的对象
    BinaryIO_Writter *BIO;
    NumsWriter numWriter;

    void scanFlow(FilePath &file, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset);

public:
    Directory_FileProcessor(std::ofstream &outFile) : outFile(outFile)
    {
        BIO = new BinaryIO_Writter(outFile);
    };
    ~Directory_FileProcessor()
    {
        delete BIO;
    };
    void directory_fileProcessor(const  std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot);
};