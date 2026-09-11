#ifndef HUFFMAN_TYPE_H
#define HUFFMAN_TYPE_H

#include <queue>
#include <stack>
#include <vector>
#include <cstdint>

// 原定义于已退役的 DataBlocks/DataBlocksManage.h；块缓冲类型别名收敛至此
namespace sfc
{
    using block_t = std::vector<unsigned char>;
}

using FreqT = uint64_t;
using CodeLenT = uint8_t;
using CodeT = std::vector<uint8_t>;

/**
 * CodeTable类：符号编码表（平坦数组，256 项，取代原 Huffmap）
 *
 * 原实现用 unordered_map<unsigned char, CharData> 存编码表，热路径（encode）
 * 每输入字节要付 3~4 次哈希探查 + 每符号一次堆上 vector 的指针追逐。
 * 改为按符号值直接索引的平坦数组后，热路径只剩两次数组 load，整表常驻 L1。
 *
 *     len[sym] == 0         该符号本块未出现
 *     1 <= len[sym] <= 64   code[sym] 为该码的右对齐（低位对齐）码字
 *     len[sym] > 64         仅 packed[sym] 有效（罕见回退路径）
 *
 * packed[] 在 len<=64 时也会写入，用于「暂存位 + 码长 > 64」时的溢出回退。
 */
struct CodeTable{
    uint64_t code[256];
    CodeLenT len[256];
    CodeT packed[256];

    CodeTable();
    void clear();
};

/**
 * HuffTreeNode：编码树节点
 */


struct HuffTreeNode{
    unsigned char data;
    FreqT freq;
    struct HuffTreeNode* left;
    struct HuffTreeNode* right;
    bool isLeaf;

    HuffTreeNode(
        const unsigned char data,
        FreqT freq,
        struct HuffTreeNode* left,
        struct HuffTreeNode* right,
        bool isLeaf=false
    );

    HuffTreeNode(const unsigned char data, FreqT freq, bool isLeaf):
        HuffTreeNode(data, freq, NULL, NULL, isLeaf) { }
/*
    HuffTreeNode(
        const char c,
        FreqT freq,
        struct HuffTreeNode* left,
        struct HuffTreeNode* right,
        bool isLeaf=false
    );*/
};

/**
 * Minheap：最小堆优先队列
 * 该容器存储树节点的指针
 */

struct CompareHeap
{
    bool operator()(const HuffTreeNode* n1, const HuffTreeNode* n2){
        // priority_queue是最大堆，要实现最小堆需要反转比较
        // 返回true表示n1优先级低于n2，会被排在后面
        // 我们希望频率小的在堆顶，所以频率大的应该优先级低
        return n1->freq > n2->freq;
    }
};

using Minheap =
std::priority_queue<HuffTreeNode*, std::vector<HuffTreeNode*>, CompareHeap>;

/**
 * PathStack：节点路径栈
 */

struct PathStack
{
    CodeT codeBlocks;
    CodeLenT codeLen;

    PathStack() : codeLen(0) { }

    void push(int bit);
    void pop();
    void writeCode(CodeTable& tab, unsigned char sym);
};

/**
 * BitHandler：比特处理器
 *     压缩时，将比特组装成字节，解压时，将字节分解为比特。
 *     记录总字节数、最后一个字节的有效位数。结束处理时写回Huffman，
 *     
 * 参数列表：
 * 
 * 函数功能：
 *     handle(code_t&, codelen_t, block_t*)：压缩处理bit流
 *     handle(unsigned char, std::vector<int>)：解压处理bit流
 *     
 */
struct BitHandler
{
    unsigned char byte;
    uint8_t bitLen;
    uint64_t byteCount;
    int valuedBits;

    BitHandler() : byte(0), bitLen(0), byteCount(0), valuedBits(0) { }

    void handle(CodeT& codeBlocks, CodeLenT codeLen, sfc::block_t&);
    // 64-bit 位缓冲快速路径：整码字一次并入、按字节批量冲刷，取代逐位移位。
    // 前置条件 bitLen + codeLen <= 64（调用方保证），否则左移会超出 64 位。
    void handleFast(uint64_t code, CodeLenT codeLen, sfc::block_t& outBlock);
    void handle(unsigned char, std::vector<uint8_t>&, uint8_t validBits = 8);
    void handleLast();
};

#endif //HUFFMAN_TYPE_H