#ifndef _THREAD_H
#define _THREAD_H

#include "MonitorTaskQueue.hpp"

#include <thread>

/**
 * 线程类
 * 封装线程与任务队列
 */

class Thread
{
public:
    Thread();
    Thread(const Thread&) = delete;
    const Thread& operator=(const Thread&) = delete;
    ~Thread();

private:
    std::thread a_thread;
    MonitorTaskQueue tasks;

    void thread_running();

public:

    template<typename T> void add_task(T&&);
    
};

template<typename T>
void Thread::add_task(T&& task)
{
    tasks.add_task(std::forward<T>(task));
}

#endif
//_THREAD_H