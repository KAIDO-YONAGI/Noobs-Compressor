#pragma once

#include <cstddef>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "../CompressorFileSystem/Commons/include/FileLibrary.h"

namespace Y_flib
{

/**
 * BufferPool —— 大块缓冲对象池（取代已退役的 DataBlocks/DataBlocksManage）
 *
 * 存在的两个理由（按重要性排序）：
 *   1. 背压与内存峰值控制：池容量 = 流水线在途块上限。
 *      读线程借不到块时在 acquire() 上挂起，被下游计算速度自然拖住，
 *      内存峰值 ≈ 容量 × 块大小，一目了然。
 *   2. 复用 8MB 块缓冲，避免反复分配/释放（该项收益本身约 1%，不是主因）。
 *
 * 语义：
 *   acquire()  池空则阻塞等待（背压点）；close() 之后抛 runtime_error
 *   release()  只清长度不清内容（vector::clear 保留容量，无 memset 开销），
 *              并唤醒一个等待的借出者
 *
 * 典型用法：全局单池——整条流水线共享一个实例（借出方读线程、归还方写线程
 * 不是同一线程，所以不做 per-thread 池）；容量按“在途上限 ≈ 2×(worker数+2)”设定。
 *
 * 线程安全：全部接口可多线程并发调用。
 */
class BufferPool
{
public:
    // capacity：池内块数上限（即流水线在途块上限）
    // blockSize：单块预分配字节数，默认 8MB（Y_flib::Constants::BUFFER_SIZE）
    explicit BufferPool(std::size_t capacity,
                        std::size_t blockSize = static_cast<std::size_t>(Y_flib::Constants::BUFFER_SIZE));

    // 借出一块（移动语义，无拷贝）。出池时长度为 0、容量为 blockSize，
    // 借用方直接 resize/assign 填充（容量足够则不触发重新分配）。
    // 池空时阻塞，直到有块归还或池被关闭（关闭后抛 std::runtime_error）。
    DataBlock acquire();

    // 归还一块：只清长度、保留容量，然后唤醒一个等待的借出者。
    // 池已关闭时静默放弃该块（内存由 vector 析构回收）。
    void release(DataBlock&& block);

    // 关闭池：唤醒所有等待者（用于流水线收尾/出错退出）。
    void close();

    std::size_t capacity() const;
    std::size_t available() const; // 当前池内可用块数（观测用）
    bool closed() const;

private:
    const std::size_t m_capacity;
    std::vector<DataBlock> m_free; // 空闲块栈（LIFO，缓存友好）
    bool m_closed = false;
    mutable std::mutex m_mtx;
    std::condition_variable m_cv;
};

} // namespace Y_flib
