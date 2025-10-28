#include "../MonitorTaskQueue.hpp"

std::function<void()> MonitorTaskQueue::get_task()
{
    std::unique_lock<std::mutex> lock(mtx);
    if(taskqueue.empty())
    {
        condition.wait(
            lock,
            [this] {
                return !taskqueue.empty();
            }
        );
    }
    auto func = std::move(taskqueue.front());
    taskqueue.pop();
    return func;
}