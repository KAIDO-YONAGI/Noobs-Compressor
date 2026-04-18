#include "../ThreadPool.h"

ThreadPool::ThreadPool()
{ }

ThreadPool::~ThreadPool()
{
}

void ThreadPool::newThread(const std::string& trdName)
{
    if(threads.find(trdName) != threads.end())
    {
        return; //接日志或异常
    }
    auto res = threads.try_emplace(trdName);
    ++threadNums;
    if(!res.second)
    {
        //接日志或异常
        threads.erase(trdName);
        --threadNums;
    }
}

void ThreadPool::delThread(const std::string& trdName)
{
    if(threads.find(trdName) == threads.end())
    {
        return;
    }
    threads.erase(trdName);
    --threadNums;
}

int ThreadPool::getThreadNums()
{
    return threadNums;
}