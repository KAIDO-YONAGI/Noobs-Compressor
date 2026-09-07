// CatalogFinalizer.h
#pragma once

#include "FileLibrary.h"
#include "ToolClasses.h"
#include "IEncryption.h"
#include <filesystem>
#include <fstream>
#include <vector>

/* CatalogFinalizer - 归档收尾器
 *
 * 功能:
 *   数据区写完后，统一处理目录区的两件收尾事：
 *     1. 回填各文件"处理后大小"预留槽（压缩循环按文件完成顺序记下的 SizeFillEntry 表）
 *     2. 目录块原地加密（读侧顺手记下、经 takeBlockSpans 移交的 BlockSpan 表）
 *
 * 归档输出文件的写入分三个阶段依次进行，任何阶段都不并发写入（本类承担阶段③）：
 *   ① HeaderWriter（ofstream，建档阶段：写文件头、目录树、各预留字段）
 *   ② DataExporter（fstream，数据阶段：逐块纯追加）
 *   ③ CatalogFinalizer（fstream，收尾阶段：回填"处理后大小"槽 + 目录区原地加密）
 * 并行化时②③归专职写线程，见《线程池调研与改造计划.md》§4.7/§7.1。
 *
 * 公共接口:
 *   finalize(): 唯一入口，先回填后加密，次序在类内焊死
 */
namespace Y_flib
{
    /* SizeFillEntry - 一条待回填的"处理后大小"记录
     * 压缩循环在文件完成时配对记下：预留槽在哪 + 要写的值 */
    struct SizeFillEntry
    {
        SlotOffset slotOffset = 0; // "处理后大小"预留字段在归档中的偏移（绝对位置）
        FileSize processedSize = 0;
    };

    class CatalogFinalizer
    {
    private:
        std::fstream archiveFile; // in|out，收尾专用句柄
        Locator locator;
        StandardsWriter standardWriter;

        /* 把各文件"处理后大小"写回目录区预留槽 */
        void backfillFileSizes(const std::vector<SizeFillEntry> &sizeEntries);

        /* 目录块原地加密：明文读出 → 加密 → 连同前置 IV 槽回写 */
        void encryptDirectoryBlocks(const std::vector<BlockSpan> &directorySpans,
                                    Y_flib::IEncryption &encryption,
                                    Y_flib::CompressionMode mode);

    public:
        /* 打开归档文件（in|out，不截断），收尾专用 */
        explicit CatalogFinalizer(const std::filesystem::path &archivePath);

        /* 唯一入口：先回填大小槽，后加密目录块。
         * 次序不可交换——加密之后槽偏移指向的就是密文了 */
        void finalize(const std::vector<SizeFillEntry> &sizeEntries,
                      const std::vector<BlockSpan> &directorySpans,
                      Y_flib::IEncryption &encryption,
                      Y_flib::CompressionMode mode);
    };
} // namespace Y_flib
