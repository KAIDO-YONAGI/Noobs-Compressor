// DataLoader.h
#pragma once

#include "FileLibrary.h"
#include "ToolClasses.h"
/* DataLoader - 二进制数据块加载器
//为非文件标准数据读取封装的读取器类，提供按块读取和按指定大小读取的功能
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
    Y_flib::DataBlock data = Y_flib::DataBlock(BUFFER_SIZE);

    Y_flib::FileSize fileSize = 0;
    std::ifstream inFile;
    bool loadIsDone = false;
    Y_flib::FileSize readed = 0;

    /* 标记读取完成状态 */
    void done();

public:
    /* 获取当前缓冲区中的数据块 */
    const Y_flib::DataBlock &getBlock() { return data; }

    /* 检查是否读取完成 */
    bool isDone() { return loadIsDone; }

    /* 打开指定文件并初始化读取状态 */
    void reset(const std::filesystem::path inPath);

    /* 按缓冲区大小读取数据块 */
    void dataLoader();

    /* 在解压流程中按指定大小读取数据块 */
    void dataLoader(Y_flib::FileSize readSize, std::ifstream &loadFile, Y_flib::DataBlock &data);

    /* 重置指针到上次读取的位置 */
    void resetByLastReaded();

    /* 默认构造函数 */
    DataLoader() {}

    /* 构造函数，打开指定文件 */
    DataLoader(const std::filesystem::path &inPath)
        : inFile(inPath, std::ios::binary) // 使用初始化列表
    {
        if (!inFile.is_open())
            throw std::runtime_error("DataLoader()-Error: Failed to open inFile Path: " + inPath.string());
    }

    /* 析构函数，自动关闭文件流 */
    ~DataLoader()
    {
        if (inFile.is_open())
        {
            inFile.close();
        }
    }
};