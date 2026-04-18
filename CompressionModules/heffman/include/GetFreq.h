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
 *    PTaskT：智能指针管理的任务包类型
 *
 * 成员函数：
 *    私有：
 *    checkTpool()：
 *        检查是否要创建/销毁线程
 *    work(int)：
 *        实际操作，调用统计字符功能
 *    genTask(int):
 *        生成packaged_task包装的work(int)任务函数，传给线程池
 *
 *    用户接口：
 *    work(DataConnector*)：
 *        将数据块指针保存在类内
 *
 * 成员变量：
 *    私有：
 *    heffman：赫夫曼算法核心
 *    tpool：线程池
 *    inBlocks：指向输入数据块列表的指针
 *
 */
class GetFreq: public Worker
{
public:
    GetFreq(Huffman*);
    ~GetFreq();

    void work(DataConnector*) override;

private:
    using PTaskT = std::shared_ptr<std::packaged_task<void()>>;
    std::shared_ptr<Huffman> heffman;
    std::unique_ptr<ThreadPool> tpool;
    sfc::blocks_t* inBlocks;

    void checkTpool();
    void work(const int&);
    PTaskT genTask(const int&);

};

#endif //GETFREQ_H
