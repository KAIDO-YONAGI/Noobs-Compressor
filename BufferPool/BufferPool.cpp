#include "BufferPool.h"

#include <stdexcept>
#include <utility>

namespace Y_flib
{

BufferPool::BufferPool(std::size_t capacity, std::size_t blockSize)
    : m_capacity(capacity)
{
    m_free.reserve(capacity);
    for (std::size_t i = 0; i < capacity; ++i)
    {
        m_free.emplace_back();
        m_free.back().reserve(blockSize); // 预分配容量、长度保持 0（出池即用语义）
    }
}

DataBlock BufferPool::acquire()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_cv.wait(lock, [this] { return !m_free.empty() || m_closed; });
    if (m_free.empty()) // 此时必然 m_closed == true
        throw std::runtime_error("BufferPool: acquire() on closed pool");
    DataBlock block = std::move(m_free.back());
    m_free.pop_back();
    return block;
}

void BufferPool::release(DataBlock&& block)
{
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_closed)
            return;
        block.clear(); // 只清长度不清内容：归还路径不做大块 memset
        m_free.push_back(std::move(block));
    }
    m_cv.notify_one();
}

void BufferPool::close()
{
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_closed = true;
    }
    m_cv.notify_all();
}

std::size_t BufferPool::capacity() const
{
    return m_capacity;
}

std::size_t BufferPool::available() const
{
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_free.size();
}

bool BufferPool::closed() const
{
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_closed;
}

} // namespace Y_flib
