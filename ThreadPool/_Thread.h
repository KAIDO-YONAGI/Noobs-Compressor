#ifndef _THREAD_H
#define _THREAD_H

#include "MonitorTaskQueue.hpp"

#include <thread>

//FIXME: Thread需要终止线程的方法。

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
    std::thread aThread;
    MonitorTaskQueue taskQueue;

    void threadRunning();

public:

    template<typename T> void addTask(T&&);
    
};

template<typename T>
void Thread::addTask(T&& task)
{
    taskQueue.addTask(std::forward<T>(task));
}

#endif
//_THREAD_H