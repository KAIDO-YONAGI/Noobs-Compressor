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
 * 类内组合的heffman是在堆上唯一的
 * 支持多线程加速
 * 
 * 实现Worker接口的work函数，当获得的数据块列表
 * 内有多个块，则启用多线程，独立处理每一个块。
 * 
 * 私有自定义类型：
 *    ptask_t：智能指针管理的任务包类型
 * 
 * 成员函数：
 *    私有：
 *    check_tpool()：
 *        检查是否要创建/销毁线程
 *    work(int)：
 *        实际操作，调用统计字符功能
 *    gen_task(int):
 *        生成packaged_task包装的work(int)任务函数，传给线程池
 * 
 *    用户接口：
 *    work(Datacmnctor*)：
 *        将数据块指针保存在类内
 * 
 * 成员变量：
 *    私有：
 *    heffman：赫夫曼算法核心
 *    tpool：线程池
 *    in_blocks：指向输入数据块列表的指针
 * 
 */
class GetFreq: public Worker 
{
public:
    GetFreq(Heffman*);
    ~GetFreq();

    void work(Datacmnctor*) override;

private:
    using ptask_t = std::shared_ptr<std::packaged_task<void()>>;
    std::shared_ptr<Heffman> heffman;
    std::unique_ptr<ThreadPool> tpool;
    sfc::blocks_t* in_blocks;
    
    void check_tpool();
    void work(const int&);
    ptask_t gen_task(const int&);
    
};

inline GetFreq::GetFreq(Heffman* heffcore):
    heffman(heffcore), tpool(new ThreadPool(MAX_NUMS_OF_THREAD))
{ }

inline GetFreq::~GetFreq()
{ }

void GetFreq::work(Datacmnctor *datacmnctor)
{
    in_blocks = datacmnctor->get_input_blocks();
    if(in_blocks->size() == 1)
        heffman->statistic_freq(0, in_blocks->at(0));
    else 
    {
        //多线程（需要阻塞主线程）
        std::vector<std::future<void>> results;
        std::vector<ptask_t> tasks;
        check_tpool();
        for(int i = 0; i < in_blocks->size(); ++i)
        {
            auto task = gen_task(i);
            tasks.push_back(task);
            results.push_back(task->get_future());
            tpool->add_task(std::to_string(i), task);
        }
        for(auto& result : results)
        {
            result.get();
        }
    }
}

void GetFreq::check_tpool()
{
    int thread_nums = tpool->get_thread_nums();
    if(in_blocks->size() == thread_nums)
        return;
    else if(in_blocks->size() < thread_nums)
    {
        for(int i = in_blocks->size(); i <= thread_nums; ++i)
        {
            tpool->del_thread(std::to_string(i));
        }
    }
    else
    {
        for(int i = thread_nums; i <= in_blocks->size(); ++i)
        {
            tpool->new_thread(std::to_string(i));
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