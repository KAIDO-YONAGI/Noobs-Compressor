#ifndef HEFFMAN_H
#define HEFFMAN_H

#include "../../hefftype/Heffman_type.h"
#include "../../Schedule/include/Worker.h"
#include <memory>

/**
 * Heffman类 压缩/解压处理模块
 * 自定义类型：
 *     Datablk_ptr：指向Connector对象内缓冲块的指针类型。
 * 
 * 参数列表：
 *     isCompressNow：标记压缩/解压模式
 *     bytecount：64位整型，记录该文件有多少字节
 * 
 *     data_blocks：缓冲块指针列表，(Datablok_ptr)指针
 *     data_blocks_out：存放输出数据的块们
 *     thread_tabs：线程在堆上的哈希表 
 *     hashtab：总哈希表
 *     treeroot：编码树的根节点
 * 
 * 函数功能：
 *     statistic_freq(线程ID, 输入缓冲块列表)：线程统计频率
 *     merge_ttabs()：合并线程提交的哈希表们
 *     gen_hefftree()：生成编码树(将根节点绑定在treeroot)
 *     gen_minheap()：生成一个包含树节点指针的优先队列。
 *     save_code_inTab()：将编码保存至哈希表
 *     run_save_code_inTab(Hefftreenode* root)：递归运行保存编码方法。
 *     encode(线程ID，bit处理器)：线程进行编码
 *     findchar(当前树指针，结果，行走方向)：根据编码树找到对应字符
 *     decode(线程ID，bit处理器)：线程进行解码，根据比特处理器每次填充
 *              的8位int列表，在树上行走，列表遍历完成保存当前树节点。
 *     
 */


class Heffman {

public:
    Heffman(int thread_nums);
    ~Heffman();

    void statistic_freq(const int& thread_id, sfc::block_t&);
    void encode(const sfc::block_t&, sfc::block_t&, BitHandler bitoutput = BitHandler());
    void decode(const sfc::block_t&, sfc::block_t&, BitHandler bitinput = BitHandler());
    void merge_ttabs();
    void gen_hefftree();
    void save_code_inTab();

    const Heffmap* getHashtab();
    void receiveHashtab(const Heffmap*);

private:
    using Datablk_ptr = sfc::block_t*;

    uint64_t bytecount;

    //sfc::blocks_t* data_blocks;
    //sfc::blocks_t* data_blocks_out; 
    Heffmaps thread_tabs;
    Heffmap hashtab;
    Hefftreenode* treeroot;
    PathStack pathStack;


    std::unique_ptr<Minheap> gen_minheap();
    void run_save_code_inTab(Hefftreenode* root);
    void findchar(Hefftreenode* now, unsigned char* result, uint8_t toward);

};

#endif //HEFFMAN_H