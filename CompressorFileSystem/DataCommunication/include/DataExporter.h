// DataExporter.hpp
#pragma once

#include "FileLibrary.h"
#include "Directory_FileProcessor.h"
#include "ToolClasses.h"
#include "BinaryIO_Writer.h"

/* DataExporter - 二进制数据块导出器
 *
 * 功能:
 *   写入加密和压缩的数据块到输出文件
 *   支持压缩流程和解压流程中的写入操作
 *   管理文件写入进度和位置更新
 *   支持记录填充位置用于解密回填
 *
 * 公共接口:
 *   exportDataToFile_Compression(): 写入压缩数据块
 *   exportDataToFile_Decompression(): 写入解压数据块
 *   thisFileIsDone(): 更新当前文件的完成位置
 *   getProcessedFileSize(): 获取已处理数据大小
 */
class DataExporter
{
private:
    std::fstream outFile;
    Locator locator;
    FileSize_uint processedFileSize = 0;

    /* 标记单个数据块处理完成并更新位置 */
    void thisBlockIsDone(DirectoryOffsetSize_uint dataSize);

public:
    /* 构造函数，打开输出文件（使用fstream支持读写） */
    DataExporter(const fs::path &outPath)
    {
        std::fstream outFile(outPath, std::ios::binary | std::ios::out | std::ios::in); // 避免截断，只能使用fstream输出
        if (!outFile)
        {
            throw std::runtime_error("DataExporter()-Error:Failed to open outFile");
        }
        this->outFile = std::move(outFile);
    };

    /* 析构函数，自动关闭输出文件 */
    ~DataExporter()
    {
        if (outFile.is_open())
        {
            outFile.close();
        }
    }

    /* 获取已处理数据的总大小 */
    FileSize_uint getProcessedFileSize() { return processedFileSize; }

    /* 更新当前文件的完成标记和位置 */
    void thisFileIsDone(FileSize_uint offsetToFill);

    /* 写入压缩数据块到输出文件 */
    void exportDataToFile_Compression(const DataBlock &data);

    /* 写入解压数据块到输出文件 */
    void exportDataToFile_Decompression(const DataBlock &data);
};