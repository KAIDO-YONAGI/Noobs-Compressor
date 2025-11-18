#include "../ThreadPool.h"

ThreadPool::ThreadPool(int t_nums):
    thread_nums(t_nums) { }


void ThreadPool::new_thread(std::string trd_name)
{
    if(threads.find(trd_name) != threads.end())
    {
        return; //接日志或异常
    }
    auto res = threads.try_emplace(trd_name);
    if(!res.second)
    {
        //接日志或异常
    }
}
