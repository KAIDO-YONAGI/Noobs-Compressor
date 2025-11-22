#include "../ThreadPool.h"

ThreadPool::ThreadPool(int t_nums):
    thread_nums(t_nums) { }


void ThreadPool::new_thread(const std::string& trd_name)
{
    if(threads.find(trd_name) != threads.end())
    {
        return; //接日志或异常
    }
    auto res = threads.try_emplace(trd_name);
    ++thread_nums;
    if(!res.second)
    {
        //接日志或异常
        threads.erase(trd_name);
        --thread_nums;
    }
}

void ThreadPool::del_thread(const std::string& trd_name)
{
    if(threads.find(trd_name) == threads.end())
    {
        return;
    }
    threads.erase(trd_name);
    --thread_nums;
}

int ThreadPool::get_thread_nums()
{
    return thread_nums;
}