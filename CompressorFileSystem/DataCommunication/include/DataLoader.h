// DataLoader.h
#pragma once

#include "FileLibrary.h"

/* DataLoader - 二进制数据块加载器
 *
 * 功能:
 *   逐块读取文件数据到缓冲区
 *   支持普通文件读取和解压流程中的指定大小读取
 *   管理读取进度和完成状态
 *   自动处理文件流生命周期
 *
 * 公共接口:
 *   dataLoader(): 主读取函数，按缓冲区大小读取
 *   getBlock(): 获取当前缓冲区数据块
 *   isDone(): 检查是否读取完成
 *   reset(): 重新初始化并打开文件
 */
class DataLoader
{
private:
    DataBlock data = DataBlock(BUFFER_SIZE);

    FileSize_uint fileSize = 0;
    std::ifstream inFile;
    bool loadIsDone = false;
    FileSize_uint readed=0;

    /* 标记读取完成状态 */
    void done();

public:
    /* 获取当前缓冲区中的数据块 */
    const DataBlock &getBlock() { return data; }

    /* 检查是否读取完成 */
    bool isDone() { return loadIsDone; }

    /* 打开指定文件并初始化读取状态 */
    void reset(const fs::path inPath);

    /* 按缓冲区大小读取数据块 */
    void dataLoader();

    /* 在解压流程中按指定大小读取数据块 */
    void dataLoader(FileSize_uint readSize, std::ifstream &decompressionFile);

    /* 设置文件总大小 */
    void setFileSize(FileSize_uint newSize) { fileSize = newSize; }

    /* 重置指针到上次读取的位置 */
    void resetByLastReaded();

    /* 默认构造函数 */
    DataLoader() {}

    /* 构造函数，打开指定文件 */
    DataLoader(const fs::path &inPath)
    {
        std::ifstream inFile(inPath, std::ios::binary);
        if (!inFile)
            throw std::runtime_error("DataLoader()-Error:Failed to open inFile Path:" + inPath.string());
        this->inFile = std::move(inFile);
    };

    /* 析构函数，自动关闭文件流 */
    ~DataLoader()
    {
        if (inFile.is_open())
        {
            inFile.close();
        }
    }
};