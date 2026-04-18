#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "../../hefftype/Heffman_type.h"
#include "../../../DataBlocks/DataBlocksManage.h"
#include <memory>
#include <stack>

/**
 * Huffman类 压缩/解压处理模块
 * 自定义类型：
 *     DataBlkPtr：指向Connector对象内缓冲块的指针类型。
 * 
 * 参数列表：
 *     isCompressNow：标记压缩/解压模式
 *     byteCount：64位整型，记录该文件有多少字节
 * 
 *     data_blocks：缓冲块指针列表，(Datablok_ptr)指针
 *     data_blocks_out：存放输出数据的块们
 *     threadTabs：线程在堆上的哈希表 
 *     hashTab：总哈希表
 *     treeRoot：编码树的根节点
 * 
 * 函数功能：
 *     statisticFreq(线程ID, 输入缓冲块列表)：线程统计频率
 *     mergeTtabs()：合并线程提交的哈希表们
 *     genHefftree()：生成编码树(将根节点绑定在treeRoot)
 *     genMinheap()：生成一个包含树节点指针的优先队列。
 *     saveCodeInTab()：将编码保存至哈希表
 *     runSaveCodeInTab(HeffTreeNode* root)：递归运行保存编码方法。
 *     encode(线程ID，bit处理器)：线程进行编码
 *     findchar(当前树指针，结果，行走方向)：根据编码树找到对应字符
 *     decode(线程ID，bit处理器)：线程进行解码，根据比特处理器每次填充
 *              的8位int列表，在树上行走，列表遍历完成保存当前树节点。
 *     
 */

class Huffman {

public:
    Huffman(int threadNums);
    ~Huffman();

    //压缩调用：statisticFreq、mergeTtabs、treeToPlatUchar↑、genHefftree、saveCodeInTab、encode
    //解压调用：spawnTree、decode
    void statisticFreq(const int threadId, const sfc::block_t&);
    void encode(const sfc::block_t&, sfc::block_t&, BitHandler bitOutput = BitHandler());
    void decode(const sfc::block_t&, sfc::block_t&, BitHandler bitInput = BitHandler(), size_t maxOutputSize = SIZE_MAX);
    void mergeTtabs();
    void genHefftree();
    void saveCodeInTab();
    //序列化编码树并输出
    void treeToPlatUchar(sfc::block_t& outBlock);
    //解析编码表并加载树
    void spawnTree(sfc::block_t& inBlock);

    HeffTreeNode* getTreeRoot();
    void receiveTreeRoot(HeffTreeNode*);

    // 调试方法：打印编码表统计信息
    void debugPrintCodeStats();

private:
    using DataBlkPtr = sfc::block_t*;

    uint64_t byteCount;

    Heffmaps threadTabs;
    Heffmap hashTab;
    HeffTreeNode* treeRoot;
    PathStack pathStack;

    std::unique_ptr<Minheap> genMinheap();
    void runSaveCodeInTab(HeffTreeNode* root);
    bool findchar(HeffTreeNode* &now, unsigned char& result, uint8_t toward);
    bool connectNode(HeffTreeNode*, HeffTreeNode*);
    void destroyTree(HeffTreeNode* node);
    size_t countLeaves(HeffTreeNode* node);
};

#endif //HUFFMAN_H