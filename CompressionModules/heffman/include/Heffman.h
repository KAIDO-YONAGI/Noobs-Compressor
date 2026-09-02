#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "../../hefftype/Heffman_type.h"
#include <memory>
#include <stack>

/**
 * Huffman类 压缩/解压处理模块
 * 
 *     freqTab：暂存频率表（统计当前块，finishFreqStat 时并入总表）
 *     hashTab：总哈希表
 *     treeRoot：编码树的根节点
 * 
 * 函数功能：
 *     statisticFreq(输入缓冲块)：统计当前块的字符频率
 *     finishFreqStat()：结束频率统计，把暂存表并入总表并清空暂存
 *     genHefftree()：生成编码树(将根节点绑定在treeRoot)
 *     genMinheap()：生成一个包含树节点指针的优先队列。
 *     saveCodeInTab()：将编码保存至哈希表
 *     runSaveCodeInTab(HeffTreeNode* root)：递归运行保存编码方法。
 *     encode(bit处理器)：编码
 *     findchar(当前树指针，结果，行走方向)：根据编码树找到对应字符
 *     decode(bit处理器)：解码，根据比特处理器每次填充
 *              的8位int列表，在树上行走，列表遍历完成保存当前树节点。
 *     
 */

class Huffman {

public:
    Huffman();
    ~Huffman();

    //压缩调用：statisticFreq、finishFreqStat、treeToPlatUchar↑、genHefftree、saveCodeInTab、encode
    //解压调用：spawnTree、decode
    void statisticFreq(const sfc::block_t&);
    void encode(const sfc::block_t&, sfc::block_t&, BitHandler bitOutput = BitHandler());
    void decode(const sfc::block_t&, sfc::block_t&, BitHandler bitInput = BitHandler(), size_t maxOutputSize = SIZE_MAX);
    void finishFreqStat();
    void genHefftree();
    void saveCodeInTab();
    //序列化编码树并输出
    void treeToPlatUchar(sfc::block_t& outBlock);
    //解析编码表并加载树
    void spawnTree(sfc::block_t& inBlock);

private:
    Heffmap freqTab;   // 暂存频率表（单个块），finishFreqStat 时并入 hashTab
    Heffmap hashTab;
    HeffTreeNode* treeRoot;
    PathStack pathStack;

    std::unique_ptr<Minheap> genMinheap();
    void runSaveCodeInTab(HeffTreeNode* root);
    bool findchar(HeffTreeNode* &now, unsigned char& result, uint8_t toward);
    bool connectNode(HeffTreeNode*, HeffTreeNode*);
    void destroyTree(HeffTreeNode* node);
};

#endif //HUFFMAN_H