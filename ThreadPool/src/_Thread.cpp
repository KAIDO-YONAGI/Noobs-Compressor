#include "../_Thread.h"

Thread::Thread()
{
    a_thread = std::thread(&Thread::thread_running, this);
}

void Thread::thread_running()
{
    while(true)
    {
        auto task = tasks.get_task();
        task();
    }
}

Thread::~Thread()
{
    if (a_thread.joinable())
        a_thread.join();
}