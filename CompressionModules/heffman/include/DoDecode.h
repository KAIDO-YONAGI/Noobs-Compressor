#ifndef DODECODE_H
#define DODECODE_H

#include <memory>
#include "Heffman.h"
#include "../../../Schedule/include/Worker.h"
#include "../../../ThreadPool/ThreadPool.h"

/**
 * 赫夫曼的解压缩功能模块
 * 类内组合的heffman在堆上唯一
 * 不支持多线程加速！
 */
class DoDecode: public Worker
{
public:
    DoDecode(Heffman*);
    ~DoDecode();

    void work(Datacmnctor*) override;

private:
    std::shared_ptr<Heffman> heffman;
    sfc::blocks_t* in_blocks;
    sfc::blocks_t* out_blocks;

};


#endif //DODECODE_H