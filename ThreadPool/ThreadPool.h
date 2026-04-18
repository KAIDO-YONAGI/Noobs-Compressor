#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "MonitorTaskQueue.hpp"
#include "_Thread.h"

#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <map>

//TODO: 考虑日志与异常处理。

/**
 * 版本一：2025.11.18
 *        基本功能开发完成。考虑日志与异常。
 *        _Thread中需要终止线程的方法
 *        MonitorTaskQueue和_Thread考虑模板化
 */

/**
 * 线程池，用于管理一组线程。
 * 该类封装了自定义线程类。提供对线程命名的功能。
 * 任务队列在线程类内部，使用管程封装。
 * 请在堆上创建并使用这个线程池
 * 若一个线程池由多个线程管理，则getThreadNums()不可用！
 * 
 * 变量:
 *     私有变量：
 *     threadNums：当前线程的数量
 *     threads：线程列表
 * 
 * 函数：
 *     私有函数：
 *     threadRunning()：线程的函数，内部是一个大循环，从任务队列取出
 *                      函数指针调用。
 *     用户接口：
 *     newThread(trdName)：创建一个命名线程
 *     delThread(trdName)：销毁一个命名线程
 *     addTask(trdName, task)：为命名线程添加任务
 *     getThreadNums(): 返回当前池内线程数量
 */

class ThreadPool
{
public:
    ThreadPool();
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    int threadNums;
    std::map<std::string, Thread> threads;

    /*
    std::vector<std::thread> threads;
    std::vector<MonitorTaskQueue> tasks;
    */

public:
    void newThread(const std::string& trdName);
    void delThread(const std::string& trdName);
    template<typename T> void addTask(std::string trdName, T&& task);
    int getThreadNums();
};

template<typename T>
void ThreadPool::addTask(std::string trdName, T&& task)
{
    if(threads.find(trdName) != threads.end())
    {
        threads[trdName].addTask(std::forward<T>(task));
    }
}


#endif //THREADPOOL_H