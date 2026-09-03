#pragma once

#include "FileLibrary.h"
#include "ToolClasses.h"
#include "EntryDetails.h"
#include <filesystem>
#include <iostream>

/* BinaryStandardWriter - 二进制目录结构序列化写入器
 *
 * 功能:
 *   扫描本地文件系统并序列化为二进制目录结构
 *   分层处理目录和文件，生成标准化的目录块
 *   支持逻辑根节点和多文件任务处理
 *   写入分隔符标记用于区分不同数据块
 *
 * 公共接口:
 *   binaryStandardWriter(): 主写入函数，扫描并序列化目录结构
 *   writeRoot(): 写入根节点（filesystem自动忽略的节点）
 *   writeLogicalRoot(): 写入逻辑根节点，用于多文件任务
 *   writeBlankSeparatedStandard(): 写入分隔符标记
 */
namespace Y_flib
{
    class BinaryStandardWriter
    {
    public:
        /* DirectoryScanCursor - 目录区写入游标
         *
         * 集中维护目录区写入的两个位置不变式（此前以 tempOffset/offset 两个裸变量
         * 散落在 EntryProcessor 与本类 8 个方法之间）：
         *   nextSlotPos —— 下一个分割标准 flag 字节的绝对偏移
         *   blockBytes  —— 当前块自分割标准之后累计的标准字节数
         * 游标只做位置算术；字节的实际写入仍在本类的各 write* 方法中。
         * 不变式：槽位大小不计入 blockBytes——nextSlotPos 永远指向
         * "拿到即可直接定位数据"的槽位 flag，无需二次变换。
         */
        struct DirectoryScanCursor
        {
            SlotOffset nextSlotPos = Constants::HEADER_SIZE; // 初值：紧跟文件头的第一个空分割标准
            BlockLength blockBytes = 0;

            /* 某条目的标准（基础尺寸+变长部分）计入当前块 */
            void accountEntry(BlockLength standardSize) { blockBytes += standardSize; }

            /* 当前块是否达到分割阈值，需要回填块长度并另起一块 */
            bool needsSeparation() const { return blockBytes >= Constants::HEADER_BUFFER_SIZE; }

            /* 块长度槽位的绝对偏移（分割标准内 flag 之后的 8 字节字段） */
            SlotOffset lengthSlotPos() const { return nextSlotPos + Constants::FLAG_SIZE; }

            /* 块已回填闭合：游标跳过块体，块内累计清零 */
            void onBlockSealed() { nextSlotPos += blockBytes; blockBytes = 0; }

            /* 已预写下一个空分割标准：游标跳过槽位本身（大小不计入 blockBytes） */
            void onSlotReserved() { nextSlotPos += Constants::SEPARATED_STANDARD_SIZE; }
        };

    private:
        StandardsWriter standardWriter;
        Locator locator;
        std::ofstream &outFile;

        /* 序列化单个目录及其子元素，写入目录标准格式 */
        void writeDirectoryStandard(EntryDetails &details, Y_flib::FileCount count, DirectoryScanCursor &cursor);

        /* 序列化单个文件的元数据，写入文件标准格式 */
        void writeFileStandard(EntryDetails &details, DirectoryScanCursor &cursor);

        /* 把当前块累计字节数回填到分割标准的长度槽位 */
        void writeSeparatedStandard(DirectoryScanCursor &cursor);

        /* 分发当前路径上的目录/文件到相应的写入处理函数，维护游标的分割/回填时序 */
        void writeStorageStandard(EntryDetails &details, EntryQueue &entryQueue, DirectoryScanCursor &cursor);

        /* 处理符号链接的序列化写入 */
        void writeSymbolLinkStandard(EntryDetails &details, DirectoryScanCursor &cursor);

        /* 统计指定目录下的文件总数（不递归） */
        Y_flib::FileCount countFilesInDirectory(const std::filesystem::path &filePathToScan);

        /* 获取指定文件的大小 */
        Y_flib::FileSize getFileSize(const std::filesystem::path &filePathToScan);

    public:
        /* 构造函数，初始化写入器并关联输出文件流 */
        BinaryStandardWriter(std::ofstream &outFile) : outFile(outFile) {};

        /* 写入逻辑根节点，用于处理多文件（目录）任务 */
        void writeLogicalRoot(const std::string &logicalRoot, const Y_flib::FileCount count, DirectoryScanCursor &cursor);

        /* 写入根节点，处理filesystem自动忽略的根目录 */
        void writeRoot(FilePath &file, const std::vector<std::string> &filePathToScan, DirectoryScanCursor &cursor);

        /* 写入空白分隔符标记 */
        void writeBlankSeparatedStandard();

        /* 写入用于加密的空白分隔符标记 */
        void writeBlankSeparatedStandardForEncryption(std::fstream &File);

        /* 主扫描函数，递归扫描并序列化目录结构到二进制格式 */
        void binaryStandardWriter(FilePath &file, EntryQueue &entryQueue, DirectoryScanCursor &cursor);
    };
} // namespace Y_flib
