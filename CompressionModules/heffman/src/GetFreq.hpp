#ifndef GETFREQ_H
#define GETFREQ_H

#include <thread>
#include <future>
#include <memory>
#include <string>
#include "../include/Heffman.h"
#include "../../../ThreadPool/ThreadPool.h"

//临时
#define MAX_NUMS_OF_THREAD 8

/**
 * 赫夫曼统计字符频率的功能模块
 * 类内组合的heffman是唯一的，并且与其他
 * 赫夫曼功能模块共同维护对heffman的引用计数。
 * 析构时，若引用计数减为零，析构heffman。
 * 
 * 实现Worker接口的work函数，当获得的数据块列表
 * 内有多个块，则启用多线程，独立处理每一个块。
 * 
 * 成员函数：
 *    私有：
 *    work(int)：
 *        实际操作，调用统计字符功能
 *    gen_task(int):
 *        生成packaged_task包装的任务函数，传给线程池
 * 
 *    用户接口：
 *    work(Datacmnctor*)：
 *        将数据块指针保存在类内
 *    
 */

class GetFreq: public Worker 
{
public:
    GetFreq(Heffman*);
    ~GetFreq();

private:
    using ptask_t = std::shared_ptr<std::packaged_task<void()>>;
    std::shared_ptr<Heffman> heffman;
    //int thread_nums; 
    std::unique_ptr<ThreadPool> tpool;
    sfc::blocks_t* in_blocks;
    
    void work(const int&);
    ptask_t gen_task(const int&);
    
public:
    void work(Datacmnctor*) override;
};

inline GetFreq::GetFreq(Heffman* heffcore):
    heffman(heffcore), tpool(new ThreadPool(MAX_NUMS_OF_THREAD))
{ }

inline GetFreq::~GetFreq()
{ }

void GetFreq::work(Datacmnctor *datacmnctor)
{
    in_blocks = datacmnctor->get_input_blocks();
    out_blocks = datacmnctor->get_output_blocks();
    if(in_blocks->size() == 1)
    {
        heffman->statistic_freq(0, in_blocks->at(0));
    }
    else 
    {
        //多线程（需要阻塞主线程）
        std::vector<std::future<void>> results;
        std::vector<ptask_t> tasks;
        for(int i = 0; i < in_blocks->size(); ++i)
        {
            auto task = gen_task(i);
            tasks.push_back(task);
            results.push_back(task->get_future());
            tpool->new_thread(std::to_string(i));
            tpool->add_task(std::to_string(i), task);
        }
        for(auto& result : results)
        {
            result.get();
        }
    }
}

void GetFreq::work(const int& i)
{
    heffman->statistic_freq(i, in_blocks->at(i)); 
}

GetFreq::ptask_t GetFreq::gen_task(const int& i)
{
    std::packaged_task<void()> *task_ptr = new std::packaged_task<void()>(
        [this, i] { this->work(i); }
    );
    ptask_t task(task_ptr);
    return task;
}

#endif //GETFREQ_H