// WriteSorter.h
#pragma once
// 按序刷盘缓冲——移植自 ThreadLab，模板化并以右值接收修掉整块拷贝问题：
// 原版 canWrite(Pack&) 把整块拷进 map，调用方弹出的原块不还池；
// 现版 canWrite(序号, T&&) 移动进 map，调用方弹出的空壳自然析构，
// 块随 map 里的正身走原路（takeInOrder 移出、写盘后归还）。
//
// 区段算法不变（min/max 连续性判据）：
//   缓冲条数 == 序号差值（连续占满） 且 恰好接在上一个已写序号之后（lastWrited == min-1）。
// 序号无洞由结构保证：单线程读端按序分配、每条消息必达写端、队列排空契约不丢，
// 最坏情形是全部消息在写端汇合——届时整段一次性凑齐刷出。
// 刷出编排由本类自理：takeInOrder 自己枚举期望区间、推进已写前缀、复位段界。
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T>
class WriteSorter
{
public:
    WriteSorter()
    {
        lastWrited = 0;
        maxSequence = 0;
        minSequence = 0;
    }

    /* 乱序日志（调试用）：不需要时保持不调用即可 */
    void logOutOfOrder(uint64_t arrivedSeq, uint64_t expectSeq)
    {
        std::cout << "============== [乱序] 收到 " << arrivedSeq
                  << ",期望 " << expectSeq
                  << ",已缓冲 " << storeMap.size() << " 条\n";
    }

    /* 缓存一条消息（移动进表），并回报当前缓冲是否已成连续段、可以整段刷出 */
    bool canWrite(uint64_t sequence, T &&message)
    {
        // 调试时可打开乱序日志（高频路径，平时必须保持注释，避免打印阻塞流水线）
        // logOutOfOrder(sequence, lastWrited + 1);

        if (storeMap.empty())
            minSequence = maxSequence = sequence; // 首条直赋
        else
        {
            minSequence = std::min(minSequence, sequence);
            maxSequence = std::max(maxSequence, sequence);
        }
        storeMap.emplace(sequence, std::move(message)); // 移动进表：零拷贝，修原版整块拷贝
        return storeMap.size() == maxSequence - minSequence + 1 && // 条数 == 序号差值——连续占满
               (lastWrited == minSequence - 1);                     // 恰好接在上一个已写序号之后
    }

    /* 取出当前整段（序号升序，移动出：零拷贝），已写前缀推进到本段末尾，段界复位。
     * 前置条件：canWrite 刚返回 true（段内序号必已缓存，取不到是调用方 bug，断言拦住） */
    std::vector<T> takeInOrder()
    {
        std::vector<T> ordered;
        if (storeMap.empty())
            return ordered;
        ordered.reserve(storeMap.size());
        for (uint64_t sequence = minSequence; sequence <= maxSequence; ++sequence)
        {
            auto it = storeMap.find(sequence);
            assert(it != storeMap.end()); // operator[] 取不到会静默塞空值，必须拦住
            ordered.push_back(std::move(it->second));
            storeMap.erase(it);
        }
        lastWrited = maxSequence; // 已写前缀推进到本段末尾
        maxSequence = 0;
        minSequence = 0;
        return ordered;
    }

    /* 停刷出口：一次性移出全部滞留消息（调用方负责归还其中的池块），簿记复位。
     * 使用场景：写端在首个任务异常处提前停刷后，尚未写出的消息由此退场 */
    std::vector<T> drainAll()
    {
        std::vector<T> remaining;
        remaining.reserve(storeMap.size());
        for (auto &entry : storeMap)
            remaining.push_back(std::move(entry.second));
        storeMap.clear();
        maxSequence = 0;
        minSequence = 0;
        return remaining;
    }

    uint64_t getSize()
    {
        return storeMap.size();
    }
    uint64_t getMaxSequence()
    {
        return maxSequence;
    }
    uint64_t getMinSequence()
    {
        return minSequence;
    }

private:
    std::unordered_map<uint64_t, T> storeMap;
    uint64_t lastWrited;  // 已写出的最大序号；初始 0 = 「0 号已写」（还没有任何消息写出）
    uint64_t maxSequence; // 当前缓冲区段的上/下界（takeInOrder/drainAll 才复位）
    uint64_t minSequence;
};
