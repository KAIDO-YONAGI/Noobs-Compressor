#ifndef GETFREQ_H
#define GETFREQ_H

#include "../include/Heffman.h"
#include <thread>

/**
 * 赫夫曼统计字符频率的功能模块
 * 类内组合的heffman是唯一的，并且与其他
 * 赫夫曼功能模块共同维护对heffman的引用计数。
 * 析构时，若引用计数减为零，析构heffman。
 * 
 * 实现Worker接口的work函数，当获得的数据块列表
 * 内有多个块，则启用多线程，独立处理每一个块。
 */

class GetFreq: public Worker 
{
public:
    GetFreq(Heffman*, int*);
    ~GetFreq();

private:
    Heffman *heffman;
    int *core_use_count;

public:
    void work(Datacmnctor*) override;
};

inline GetFreq::GetFreq(Heffman* heffcore, int *count):
    heffman(heffcore), core_use_count(count) 
    {  
        ++*core_use_count;
    }

inline GetFreq::~GetFreq()
{
    --*core_use_count;
    if(*core_use_count == 0){
        delete core_use_count;
        delete heffman;
    }
}

void GetFreq::work(Datacmnctor *datacmnctor)
{
    sfc::blocks_t* inputblocks = datacmnctor->get_input_blocks();
    
    if(inputblocks->size() == 1)
    {
        heffman->statistic_freq(1, inputblocks);
    } 
    else 
    {
        //多线程
    }
}

#endif //GETFREQ_H