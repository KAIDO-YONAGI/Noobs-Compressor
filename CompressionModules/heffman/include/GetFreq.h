#ifndef GETFREQ_H
#define GETFREQ_H

#include <thread>
#include <future>
#include <memory>
#include <string>
#include "../include/Heffman.h"
#include "../../../ThreadPool/ThreadPool.h"
#include "../../../Schedule/include/Worker.h"

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

#endif //GETFREQ_H