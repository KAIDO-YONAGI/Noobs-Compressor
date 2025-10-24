#ifndef MONITORTASKQUEUE_H
#define MONITORTASKQUEUE_H

/**
 * 管程，封装了线程池每个线程的任务队列。
 * 
 */

#include <mutex>


class MonitorTaskQueue 
{
public:
    MonitorTaskQueue();
    ~MonitorTaskQueue();
    MonitorTaskQueue(const MonitorTaskQueue&) = delete;
    MonitorTaskQueue& operator=(MonitorTaskQueue&&) = delete;

private:
    


};

#endif //MONITORTASKQUEUE_H
