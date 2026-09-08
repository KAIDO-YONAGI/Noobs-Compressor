// PipelineMessages.h
#pragma once
// 压缩/解压两条流水线共用的任务与结果消息。
//
// 序号规则（两側同构）：全局递增，由读线程单线程分配；块与文件界消息都占号、
// 异常消息不跳号——无洞由结构保证（读端按序分配、工人每任务必产出、队列排空
// 契约不丢），这是写线程 WriteSorter 区段连续性判据成立的前提；最坏情形是
// 全部消息在写端汇合一次刷出。
//
// 池块随消息走全链路（防死锁不变量，两側一致）：读线程是唯一借出方，写线程是
// 唯一归还方，工人零池调用——压缩側以加密输出原地覆写明文输入，解压側以解压
// 输出原地覆写密文输入，块的所有权随消息移交写线程。
#include <cstdint>
#include <exception>
#include <filesystem>

#include "../CompressorFileSystem/Commons/include/FileLibrary.h"

// ======================== 压缩流水线 ========================

/* 读线程 → 工人：一个输入块（缓冲池借出，装明文） */
struct BlockTask
{
    std::uint64_t sequence = 0;
    Y_flib::DataBlock inputData; // 池块；装载器内部缓冲会被覆写，读线程必须复制进池块
};

/* 工人/读线程 → 写线程：结果队列元素。
 * 一个结构两种载荷，kind 标签区分，不搞 variant 分发：
 *   Block      —— 一个块的两段加密产物（写序固定：元数据在前、数据在后）；
 *   Settlement —— 一个文件完成（空文件只有它，不写任何字节），槽偏移随消息直送
 *                 写线程攒 SizeFillEntry 表，免去读写两侧对账单。 */
struct PipelineMessage
{
    enum class Kind
    {
        Block,
        Settlement
    };

    std::uint64_t sequence = 0; // 定序键（WriteSorter 依此重排）
    Kind kind = Kind::Block;

    // —— kind == Block ——
    Y_flib::DataBlock encryptedMetadata; // KB 级小对象，不进缓冲池
    Y_flib::DataBlock encryptedData;     // 池块本体（工人已原地变换为密文），写线程写盘后归还
    std::exception_ptr exception;        // 工人捕获的任务异常；非空时写线程按序重抛并停写

    // —— kind == Settlement ——
    Y_flib::SlotOffset processedSizeOffset = 0; // 目录区「处理后大小」预留槽的绝对偏移
};

// ======================== 解压流水线 ========================

/* 读线程 → 工人：一个块对的密文（元数据块 + 数据块，与归档物理顺序一致） */
struct DecompressTask
{
    std::uint64_t sequence = 0;

    /* 本块解压输出上限。读线程预计算：min(BUFFER_SIZE, originalSize − 已产字节)。
     * 与串行版的 running-remaining 数学等价——压缩侧按 BUFFER_SIZE 切块，非末块
     * 的真实剩余 ≥8MB 且输出 ≤8MB（上限不截合法输出），末块上限即精确尾长 */
    Y_flib::FileSize expectedOriginal = 0;

    Y_flib::DataBlock encryptedMetadata; // KB 级小对象，不进缓冲池
    Y_flib::DataBlock encryptedData;     // 池块本体（密文），工人原地覆写为明文后继续前行
};

/* 工人/读线程 → 写线程：解压结果队列元素。
 *   Block     —— 一个块对的解压产物（明文池块）；
 *   FileStart —— 一个输出文件的开启（空文件只有它，不写任何字节）。紧跟上一文件
 *               最后一块、先于本文件首块：写线程按序号见到它即结算上一文件并开
 *               新输出文件（读线程直发结果队列，不经工人）。 */
struct DecompressionMessage
{
    enum class Kind
    {
        Block,
        FileStart
    };

    std::uint64_t sequence = 0; // 定序键（WriteSorter 依此重排）
    Kind kind = Kind::Block;

    // —— kind == Block ——
    Y_flib::DataBlock decompressedData; // 池块本体（明文），写线程写盘后归还
    std::exception_ptr exception;       // 工人捕获的任务异常；非空时写线程按序重抛并停写

    // —— kind == FileStart ——
    std::filesystem::path outputPath; // 输出文件绝对路径
    Y_flib::FileSize originalSize = 0;
};
