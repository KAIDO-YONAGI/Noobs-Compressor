// DataExporter.hpp
#pragma once

#include "FileLibrary.h"
#include "ToolClasses.h"
#include "FileSystemUtils.h"

/* DataExporter - 二进制数据块导出器
//为非文件标准数据写入封装的写入器类，提供按块写入和按指定大小写入的功能
 *
 * 功能:
 *   写入加密和压缩的数据块到输出文件
 *   支持压缩流程和解压流程中的写入操作
 *   纯追加写入：写指针只在文件末尾定位，从不 seek 回改
 *   自行累加当前文件的处理后大小，文件完成时由调度者取走
 *
 * 归档输出文件的写入分三个阶段依次进行，任何阶段都不并发写入（本类承担阶段②）：
 *   ① HeaderWriter（ofstream，建档阶段：写文件头、目录树、各预留字段）
 *   ② DataExporter（fstream，数据阶段：逐块纯追加，块长度直写真实值）
 *   ③ CatalogFinalizer（fstream，收尾阶段：回填"处理后大小"槽 + 目录区原地加密）
 * 并行化时②③归专职写线程，见《线程池调研与改造计划.md》§7.1。
 *
 * 公共接口:
 *   exportCompressedData(): 写入压缩数据块
 *   exportDecompressedData(): 写入解压数据块
 *   currentFileProcessedSize(): 查询当前文件累计的处理后大小
 *   startNextFile(): 结束当前文件的统计，从 0 开始累计下一文件
 */
namespace Y_flib
{
    class DataExporter
    {
    private:
        std::fstream outFile;
        Locator locator;
        StandardsWriter standardWriter;
        Y_flib::FileSize processedFileSize = 0;

    public:
        /* 构造函数，打开输出文件（使用fstream支持读写） */
        DataExporter(const std::filesystem::path &outPath)
        {
            // 先检查文件是否存在
            if (!FileSystemUtils::exists(outPath))
            {
                const std::string utf8Path = EncodingUtils::pathToUtf8(outPath);
                throw std::runtime_error("DataExporter()-Error:File does not exist: " + utf8Path +
                                         "\nPath length: " + std::to_string(utf8Path.size()));
            }

            // 归档文件自身也可能位于深层目录，打开流前统一转换实际 I/O 路径。
            outFile.open(
                FileSystemUtils::pathForIo(outPath),
                std::ios::binary | std::ios::out | std::ios::in);
            if (!outFile)
            {
                const std::string utf8Path = EncodingUtils::pathToUtf8(outPath);
                throw std::runtime_error("DataExporter()-Error:Failed to open outFile: " + utf8Path +
                                         "\nPath length: " + std::to_string(utf8Path.size()) +
                                         "\nPossible reasons: path too long (>260 chars), permission denied, or file locked");
            }
        }

        /* fstream 自身负责关闭文件，无需手写析构函数。 */
        ~DataExporter() = default;

        /* 查询当前文件累计的处理后大小（块字节数直接累加，加密块含 IV 也按写入值计） */
        Y_flib::FileSize currentFileProcessedSize() const { return processedFileSize; }

        /* 结束当前文件的统计，从 0 开始累计下一文件 */
        void startNextFile() { processedFileSize = 0; }

        /* 写入压缩数据块到输出文件 */
        void exportCompressedData(const Y_flib::DataBlock &data);

        /* 写入解压数据块到输出文件 */
        void exportDecompressedData(const Y_flib::DataBlock &data);
    };
} // namespace Y_flib
