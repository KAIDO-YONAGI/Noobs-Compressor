#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <condition_variable>
#include <vector>
#include <queue>

/**
 * 线程池，用于管理一组线程。
 * 禁用拷贝操作。
 * 该类封装了线程的创建与销毁。提供对线程命名的功能。
 * 每个线程拥有一个任务队列，对外只开放对指定命名添加任务的功能。
 * 任务队列使用管程封装。
 * 
 * 私有变量:
 *     thread_nums：线程的数量
 *     taskqueCv：列表存储每个队列的条件变量
 *     threads：线程列表
 *     tasks：任务队列列表
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
    std::vector< std::condition_variable > taskqueCv;
    std::vector<std::thread> threads;
    std::vector< std::queue<> > tasks;

public:
    template<typename T>
    void add_task(T&& task);

};

ThreadPool::ThreadPool(int t_nums):
    thread_nums(t_nums)
{
    threads.reserve(t_nums);
}

#endif //THREADPOOL_H