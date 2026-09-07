// PipelineMessages.h
#pragma once
// 压缩流水线的任务/结果消息。
//
// 序号规则：全局递增，由读线程单线程分配；块与文件结算都占号、异常消息也不跳号。
// 序号「无洞」由结构保证（读端按序分配、工人每任务必产出、队列排空契约不丢），
// 这是写线程 WriteSorter 区段连续性判据成立的前提；最坏情形是全部消息在写端汇合。
#include <cstdint>
#include <exception>

#include "../CompressorFileSystem/Commons/include/FileLibrary.h"

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
 *                 写线程攒 SizeFillEntry 表，免去读写两侧对账单。
 * 池块随消息走全链路：读线程借出装明文 → 工人加密原地覆写 → 写线程写盘后归还。
 * 读线程是唯一借出方、写线程是唯一归还方，依赖图无环（ThreadLab 同构）。 */
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
