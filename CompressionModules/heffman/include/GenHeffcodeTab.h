#ifndef GENHEFFCODEDTAB_H
#define GENHEFFCODEDTAB_H

#include <memory>
#include <stack>
#include "Heffman.h"
#include "../../hefftype/Heffman_type.h"
#include "../../../Schedule/include/Datacmnctor.h"
#include "../../../Schedule/include/Worker.h"

/**
 * 赫夫曼功能模块之一：生成编码表。
 * 先合并多线程频率统计结果，生成编码树，最后
 * 完善编码表，并将其扁平化发送到数据块
 * 不支持多线程加速
 */
class GenHeffcodeTab: public Worker
{
public:
    GenHeffcodeTab(Heffman*);
    ~GenHeffcodeTab() = default;

    void work(Datacmnctor*) override;

private:
    std::shared_ptr<Heffman> heffman;
    Hefftreenode *root;
    sfc::blocks_t *out_blocks;

    void tree_to_plat_uchar(sfc::block_t&);
};


#endif //GENHEFFCODEDTAB_H