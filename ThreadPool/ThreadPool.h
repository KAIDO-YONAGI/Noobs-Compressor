#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "../namespace/namespace_sfc.h"
#include "MonitorTaskQueue.hpp"
#include "_Thread.h"

#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <map>

/**
 * 线程池，用于管理一组线程。
 * 禁用拷贝操作。
 * 该类封装了自定义线程类。提供对线程命名的功能。
 * 任务队列在线程类内部，使用管程封装。
 * 请在堆上创建并使用这个线程池
 * 
 * 变量:
 *     私有变量：
 *     thread_nums：线程的数量
 *     threads：线程列表
 * 
 * 函数：
 *     私有函数：
 *     thread_running()：线程的函数，内部是一个大循环，从任务队列取出
 *                      函数指针调用。
 */

class ThreadPool
{
public:
    ThreadPool(int);
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    int thread_nums;
    std::map<std::string, Thread> threads;

    /*
    std::vector<std::thread> threads;
    std::vector<MonitorTaskQueue> tasks;
    */

public:
    void new_thread(std::string trd_name);
    template<typename T> void add_task(std::string trd_name, T&& task);

};

template<typename T>
void ThreadPool::add_task(std::string trd_name, T&& task)
{
    
}

#endif //THREADPOOL_H