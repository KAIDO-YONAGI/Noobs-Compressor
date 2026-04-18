#include "../_Thread.h"

Thread::Thread()
{
    aThread = std::thread(&Thread::threadRunning, this);
}

void Thread::threadRunning()
{
    while(true)
    {
        auto task = taskQueue.getTask();
        task();
    }
}

Thread::~Thread()
{
    if (aThread.joinable())
        aThread.join();
}