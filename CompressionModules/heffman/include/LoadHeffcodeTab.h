#ifndef LOADHEFFCODETAB_H
#define LOADHEFFCODETAB_H

#include <memory>
#include <stack>
#include "heffman.h"
#include "../../Schedule/include/Datacmnctor.h"
#include "../../Schedule/include/Worker.h"
#include "../../hefftype/Heffman_type.h"

/**
 * 赫夫曼功能模块之一：接受编码表（从文件中获
 * 取），并构建解码树。
 * 
 * 成员函数：
 *   对外接口：
 *     work(Datacmnctor*)：
 *      运行功能
 * 
 *   私有：
 *     spawn_tree(sfc::block_t&):
 *      将序列化的heffman树还原，并返回其根节点
 *     connectNode(parent, child)：
 *      尝试连接父子节点，返回bool
 *     
 */
class LoadHeffcodeTab: public Worker
{
public:
    LoadHeffcodeTab(Heffman*);
    ~LoadHeffcodeTab() = default;

    void work(Datacmnctor*) override;

private:
    std::shared_ptr<Heffman> heffman;
    Hefftreenode *root;
    sfc::blocks_t *in_blocks;

    Hefftreenode* spawn_tree(sfc::block_t&);
    bool connectNode(Hefftreenode*, Hefftreenode*);
};

#endif //LOADHEFFCODETAB_H