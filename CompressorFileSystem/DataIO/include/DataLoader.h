// DataLoader.h
#pragma once

#include "FileLibrary.h"
#include "ToolClasses.h"
#include "FileSystemUtils.h"
/* DataLoader - 文件数据块加载器
 *
 * 功能:
 *   逐块读取文件数据到缓冲区
 *   管理读取进度和完成状态
 *   自动处理文件流生命周期
 *
 * 公共接口:
 *   dataLoader(): 主读取函数，按缓冲区大小读取
 *   getBlock(): 获取当前缓冲区数据块
 *   isDone(): 检查是否读取完成
 *   reset(): 重新初始化并打开文件
 */
namespace Y_flib
{
    class DataLoader
    {
    private:
        Y_flib::DataBlock data;

        Y_flib::FileSize fileSize = 0;
        std::ifstream inFile;
        bool loadIsDone = false;
        Y_flib::FileSize readCount = 0;

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

        /* 重置指针到上次读取的位置 */
        void resetByLastRead();

        /* 默认构造函数 */
        DataLoader() {}

        /* 构造函数，打开指定文件 */
        DataLoader(const std::filesystem::path &inPath)
            : inFile(FileSystemUtils::pathForIo(inPath), std::ios::binary) // 转换为支持超长路径的实际 I/O 路径
        {
            if (!inFile.is_open())
                throw std::runtime_error("DataLoader()-Error: Failed to open inFile Path: " + EncodingUtils::pathToUtf8(inPath));
        }

        /* ifstream 自身负责关闭文件。 */
        ~DataLoader() = default;
    };
} // namespace Y_flib
