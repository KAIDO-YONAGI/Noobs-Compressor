#ifndef MONITORTASKQUEUE_H
#define MONITORTASKQUEUE_H

#include "../namespace/namespace_sfc.h"

#include <mutex>
#include <condition_variable>

namespace sfc
{
    using task_queue = std::queue< std::function<void()> >;
}

/**
 * 任务队列的管程。
 * 封装了线程池每个线程的任务队列，以及
 * 每个线程的条件变量。当线程取出任务时发现队列空，
 * 则休眠。队列被添加，则唤醒
 * 
 * 成员变量：
 *     私有变量：
 *     mtx：锁
 *     condition：条件变量
 *     taskqueue：任务队列，存储被包装的函数
 * 
 * 成员函数：
 *     对外提供：
 *     add_task()：添加任务，并尝试唤醒线程
 *     get_task()：取出任务，若队列空挂起该线程
 */

class MonitorTaskQueue 
{
public:
    MonitorTaskQueue() = default;
    ~MonitorTaskQueue();
    MonitorTaskQueue(const MonitorTaskQueue&) = delete;
    MonitorTaskQueue& operator=(MonitorTaskQueue&&) = delete;

private:
    std::mutex mtx;
    std::condition_variable condition;    
    sfc::task_queue taskqueue;

public:
    template<typename T> void add_task(T&& task);
    std::function<void()> get_task();

};

template<typename T>
void MonitorTaskQueue::add_task(T&& task)
{
    std::unique_lock<std::mutex> lock(mtx);
    taskqueue.push(std::function<void>(std::forward<T>(task)));
    condition.notify_one();
}

//这里有模版生成约束

#endif //MONITORTASKQUEUE_H
