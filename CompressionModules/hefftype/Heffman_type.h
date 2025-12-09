#ifndef HEFFMAN_TYPE_H
#define HEFFMAN_TYPE_H

#include <queue>
#include <stack>
#include <map>
#include <vector>
#include <cstdint>
#include "../../../DataBlocks/DataBlocksManage.h"

using freq_t = uint64_t;
using codelen_t = uint8_t;
using code_t = std::vector<uint8_t>;

/**
 * Chardata类：存储一个字符的编码信息
 * 参数列表：
 *     freq：频率
 *     codelen：编码长度
 *     code：编码
 * 函数功能：
 *     add()：频率加1
 *     add(const Chardata&)：合并频率到此对象
 */

struct Chardata{
    Chardata();

    freq_t freq;
    codelen_t codelen;
    code_t code;

    void add();
    void add(const Chardata&);
};

/**
 * Heffmap类：字符到字符信息的映射
 */

typedef  
std::map<
    unsigned char, 
    Chardata
>
Heffmap;

/**
 * Heffmaps: 存储哈希表的列表
 */

typedef std::vector<Heffmap> Heffmaps;

/**
 * Hefftreenode：编码树节点
 */


struct Hefftreenode{
    unsigned char data;
    freq_t freq;
    struct Hefftreenode* left;
    struct Hefftreenode* right;
    bool isleaf;

    Hefftreenode(
        const unsigned char data, 
        freq_t freq, 
        struct Hefftreenode* left, 
        struct Hefftreenode* right, 
        bool isleaf=false
    );    

    Hefftreenode(const unsigned char data, freq_t freq, bool isleaf):
        Hefftreenode(data, freq, NULL, NULL, isleaf) { }
/*
    Hefftreenode(
        const char c, 
        freq_t freq, 
        struct Hefftreenode* left, 
        struct Hefftreenode* right, 
        bool isleaf=false
    );*/
};

/**
 * Minheap：最小堆优先队列
 * 该容器存储树节点的指针
 */

struct CompareHeap
{
    bool operator()(const Hefftreenode* n1, const Hefftreenode* n2){
        return n1->freq < n2->freq;
    }
};

using Minheap = 
std::priority_queue<Hefftreenode*, std::vector<Hefftreenode*>, CompareHeap>;

/**
 * PathStack：节点路径栈
 */

struct PathStack
{
    code_t codeblocks;
    codelen_t codelen;

    void push(int bit);
    void pop();
    void writecode(Chardata& cdata);
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
    uint8_t bitlen;
    uint64_t bytecount;
    int valued_bits;

    BitHandler() : byte(0), bitlen(0), bytecount(0), valued_bits(0) { }

    void handle(code_t& codeblocks, codelen_t codelen, sfc::block_t&);
    void handle(unsigned char, std::vector<uint8_t>&);
    void handle_last();
};

#endif //HEFFMAN_TYPE_H