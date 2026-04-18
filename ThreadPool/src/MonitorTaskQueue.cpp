#include "../MonitorTaskQueue.hpp"

MonitorTaskQueue::~MonitorTaskQueue()
{
}

std::function<void()> MonitorTaskQueue::getTask()
{
    std::unique_lock<std::mutex> lock(mtx);
    if(taskQueue.empty())
    {
        condition.wait(
            lock,
            [this] {
                return !taskQueue.empty();
            }
        );
    }
    auto func = std::move(taskQueue.front());
    taskQueue.pop();
    return func;
}