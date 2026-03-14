// DataExporter.hpp
#pragma once

#include "FileLibrary.h"
#include "EntryProcessor.h"
#include "ToolClasses.h"
#include "BinaryStandardWriter.h"

/* DataExporter - 二进制数据块导出器
//为非文件标准数据写入封装的写入器类，提供按块写入和按指定大小写入的功能
 *
 * 功能:
 *   写入加密和压缩的数据块到输出文件
 *   支持压缩流程和解压流程中的写入操作
 *   管理文件写入进度和位置更新
 *   支持记录填充位置用于解密回填
 *
 * 公共接口:
 *   exportCompressedData(): 写入压缩数据块
 *   exportDecompressedData(): 写入解压数据块
 *   thisFileIsDone(): 更新当前文件的完成位置
 *   getProcessedY_flib::FileSize(): 获取已处理数据大小
 */
class DataExporter
{
private:
    std::fstream outFile;
    Locator locator;
    StandardsWriter standardWriter;
    Y_flib::FileSize processedFileSize = 0;

    /* 标记单个数据块处理完成并更新位置 */
    void thisBlockIsDone(Y_flib::DirectoryOffsetSize dataSize);

public:
    /* 构造函数，打开输出文件（使用fstream支持读写） */
    DataExporter(const std::filesystem::path &outPath)
    {
        // 先检查文件是否存在
        if (!std::filesystem::exists(outPath))
        {
            throw std::runtime_error("DataExporter()-Error:File does not exist: " + outPath.string() +
                                    "\nPath length: " + std::to_string(outPath.string().size()));
        }

        std::fstream outFile(outPath, std::ios::binary | std::ios::out | std::ios::in);
        if (!outFile)
        {
            throw std::runtime_error("DataExporter()-Error:Failed to open outFile: " + outPath.string() +
                                    "\nPath length: " + std::to_string(outPath.string().size()) +
                                    "\nPossible reasons: path too long (>260 chars), permission denied, or file locked");
        }
        this->outFile = std::move(outFile);
    }

    /* 析构函数，自动关闭输出文件 */
    ~DataExporter()
    {
        if (outFile.is_open())
        {
            outFile.close();
        }
    }

    /* 获取已处理数据的总大小 */
    Y_flib::FileSize getProcessedFileSize() { return processedFileSize; }

    /* 更新当前文件的完成标记和位置 */
    void thisFileIsDone(Y_flib::FileSize offsetToFill);

    /* 写入压缩数据块到输出文件 */
    void exportCompressedData(const Y_flib::DataBlock &data);

    /* 写入解压数据块到输出文件 */
    void exportDecompressedData(const Y_flib::DataBlock &data);
};