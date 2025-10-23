#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <vector>

class ThreadPool
{
public:
    ThreadPool();
    ~ThreadPool();

private:
    std::vector<std::thread> threads;
};

#endif //THREADPOOL_H