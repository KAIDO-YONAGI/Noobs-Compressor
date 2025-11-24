#ifndef GENHEFFCODEDTAB_H
#define GENHEFFCODEDTAB_H

#include <memory>
#include <typeinfo>
#include "Heffman.h"
#include "../../../DataBlocks/include/HefftabBlock.h"
#include "../../hefftype/Heffman_type.h"
#include "../../../Schedule/include/Datacmnctor.h"
#include "../../Schedule/include/Worker.h"
#include "../../../ThreadPool/ThreadPool.h"

/**
 * 赫夫曼功能模块之一：生成编码表。
 * 先合并多线程频率统计结果，生成编码树，最后
 * 完善编码表，发送给HefftabBlock
 */
class GenHeffcodeTab: public Worker
{
public:
    GenHeffcodeTab(Heffman*);
    ~GenHeffcodeTab();

    void work(Datacmnctor*) override;

private:
    std::shared_ptr<Heffman> heffman;

};


#endif //GENHEFFCODEDTAB_H