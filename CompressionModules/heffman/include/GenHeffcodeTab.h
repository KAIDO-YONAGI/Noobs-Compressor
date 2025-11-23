#ifndef GENHEFFCODEDTAB_H
#define GENHEFFCODEDTAB_H

#include <memory>
#include "Heffman.h"
#include "../../Schedule/include/Worker.h"
#include "../../../ThreadPool/ThreadPool.h"

//TODO: 需要合并多线程结果，生成编码表，将其写到输出块

class GenHeffcodeTab: public Worker
{
public:
    GenHeffcodeTab(/* args */);
    ~GenHeffcodeTab();

    void work(Datacmnctor*) override;

private:
    std::shared_ptr<Heffman> heffman;
    sfc::block_t out_block;

    void outputTab(sfc::block_t&);
};


#endif //GENHEFFCODEDTAB_H