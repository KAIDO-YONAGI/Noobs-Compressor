#ifndef HUFFMAN_TYPE_H
#define HUFFMAN_TYPE_H

#include <queue>
#include <stack>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "../../DataBlocks/DataBlocksManage.h"

using FreqT = uint64_t;
using CodeLenT = uint8_t;
using CodeT = std::vector<uint8_t>;

/**
 * CharData类：存储一个字符的编码信息
 * 参数列表：
 *     freq：频率
 *     codeLen：编码长度
 *     code：编码
 * 函数功能：
 *     add()：频率加1
 *     add(const CharData&)：合并频率到此对象
 */

struct CharData{
    CharData();

    FreqT freq;
    CodeLenT codeLen;
    CodeT code;

    void add();
    void add(const CharData&);
};

/**
 * Heffmap类：字符到字符信息的映射
 */

typedef
std::unordered_map<
    unsigned char,
    CharData
>
Heffmap;

/**
 * Heffmaps: 存储哈希表的列表
 */

typedef std::vector<Heffmap> Heffmaps;

/**
 * HeffTreeNode：编码树节点
 */


struct HeffTreeNode{
    unsigned char data;
    FreqT freq;
    struct HeffTreeNode* left;
    struct HeffTreeNode* right;
    bool isLeaf;

    HeffTreeNode(
        const unsigned char data,
        FreqT freq,
        struct HeffTreeNode* left,
        struct HeffTreeNode* right,
        bool isLeaf=false
    );

    HeffTreeNode(const unsigned char data, FreqT freq, bool isLeaf):
        HeffTreeNode(data, freq, NULL, NULL, isLeaf) { }
/*
    HeffTreeNode(
        const char c,
        FreqT freq,
        struct HeffTreeNode* left,
        struct HeffTreeNode* right,
        bool isLeaf=false
    );*/
};

/**
 * Minheap：最小堆优先队列
 * 该容器存储树节点的指针
 */

struct CompareHeap
{
    bool operator()(const HeffTreeNode* n1, const HeffTreeNode* n2){
        // priority_queue是最大堆，要实现最小堆需要反转比较
        // 返回true表示n1优先级低于n2，会被排在后面
        // 我们希望频率小的在堆顶，所以频率大的应该优先级低
        return n1->freq > n2->freq;
    }
};

using Minheap =
std::priority_queue<HeffTreeNode*, std::vector<HeffTreeNode*>, CompareHeap>;

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
    void writeCode(CharData& cdata);
};

/**
 * BitHandler：比特处理器
 *     压缩时，将比特组装成字节，解压时，将字节分解为比特。
 *     记录总字节数、最后一个字节的有效位数。结束处理时写回Heffman，
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
    void handle(unsigned char, std::vector<uint8_t>&, uint8_t validBits = 8);
    void handleLast();
};

#endif //HUFFMAN_TYPE_H