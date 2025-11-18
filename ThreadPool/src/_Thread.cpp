#include "../_Thread.h"

Thread::Thread()
{
    a_thread = std::thread(thread_running);
}

void Thread::thread_running()
{
    while(true)
    {
        auto task = tasks.get_task();
        task();
    }
}