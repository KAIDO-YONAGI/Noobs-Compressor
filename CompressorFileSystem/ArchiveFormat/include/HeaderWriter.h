// HeaderWriter.h
#pragma once

#include "FileLibrary.h"
#include "EntryProcessor.h"

/* HeaderWriter - 当前归档格式的文件头和目录写入器
 *
 * 功能:
 *   写入当前版本文件头并序列化目录结构
 *   写入端始终生成 Constants::VERSION 指定的当前格式
 *
 * 归档输出文件的写入分三个阶段依次进行，任何阶段都不并发写入（本类承担阶段①）：
 *   ① HeaderWriter（ofstream，建档阶段：写文件头、目录树、各预留字段）
 *   ② DataExporter（fstream，数据阶段：逐块追加，并回写每块长度、每文件大小）
 *   ③ BinaryStandardLoader::encryptHeaderBlock（fstreamForRefill，收尾阶段：目录区原地加密）
 * 并行化时②③归专职写线程，见《线程池调研与改造计划.md》§7.1。
 */
namespace Y_flib
{
    class HeaderWriter
    {
    private:
        /* 写入文件头固定字段并回填头部长度。 */
        void writeHeader(
            std::ofstream &outFile,
            const std::filesystem::path &fullOutPath,
            Y_flib::CompressionMode mode);

        /* 序列化目录结构并回填目录结束偏移。 */
        void writeDirectory(
            std::ofstream &outFile,
            const std::vector<std::string> &filePathToScan,
            const std::string &logicalRoot);

    public:
        /* 创建归档并依次写入魔数、文件头、目录区和尾部魔数。 */
        void headerWriter(
            const std::vector<std::string> &filePathToScan,
            const std::string &outputFilePath,
            const std::string &logicalRoot,
            Y_flib::CompressionMode mode = Y_flib::CompressionMode::HuffmanAES);
    };
} // namespace Y_flib
