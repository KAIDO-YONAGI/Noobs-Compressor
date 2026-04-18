#ifndef LOADHEFFCODETAB_H
#define LOADHEFFCODETAB_H

#include <memory>
#include <stack>
#include "Heffman.h"
#include "../../Schedule/include/Datacmnctor.h"
#include "../../Schedule/include/Worker.h"
#include "../../hefftype/Heffman_type.h"

/**
 * 赫夫曼功能模块之一：接受编码表（从文件中获
 * 取），并构建解码树。
 *
 * 成员函数：
 *   对外接口：
 *     work(DataConnector*)：
 *      运行功能
 *
 *   私有：
 *     spawnTree(sfc::block_t&):
 *      将序列化的Huffman树还原，并返回其根节点
 *     connectNode(parent, child) ：
 *      尝试连接父子节点，返回bool
 *
 */
class LoadHeffcodeTab: public Worker
{
public:
    LoadHeffcodeTab(Huffman*);
    ~LoadHeffcodeTab() = default;

    void work(DataConnector*) override;

private:
    std::shared_ptr<Huffman> heffman;
    HeffTreeNode *root;
    sfc::blocks_t *inBlocks;

    HeffTreeNode* spawnTree(sfc::block_t&);
    bool connectNode(HeffTreeNode*, HeffTreeNode*);
};

#endif //LOADHEFFCODETAB_H
