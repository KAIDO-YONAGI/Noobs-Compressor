#ifndef GENHEFFCODEDTAB_H
#define GENHEFFCODEDTAB_H

#include <memory>
#include <stack>
#include "Heffman.h"
#include "../../hefftype/Heffman_type.h"
#include "../../../Schedule/include/Datacmnctor.h"
#include "../../Schedule/include/Worker.h"

/**
 * 赫夫曼功能模块之一：生成编码表。
 * 先合并多线程频率统计结果，生成编码树，最后
 * 完善编码表，并将其扁平化发送到数据块
 * 不支持多线程加速
 */
class GenHeffcodeTab: public Worker
{
public:
    GenHeffcodeTab(Huffman*);
    ~GenHeffcodeTab() = default;

    void work(DataConnector*) override;

private:
    std::shared_ptr<Huffman> heffman;
    HeffTreeNode *root;
    sfc::blocks_t *outBlocks;

    void treeToPlatUchar(sfc::block_t&);
};


#endif //GENHEFFCODEDTAB_H
