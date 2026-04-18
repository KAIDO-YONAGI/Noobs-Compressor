#include "../include/GetFreq.h"

GetFreq::GetFreq(Huffman* heffcore):
    heffman(heffcore), tpool(new ThreadPool())
{ }

GetFreq::~GetFreq()
{ }

void GetFreq::work(DataConnector *datacmnctor)
{
    inBlocks = datacmnctor->getInputBlocks();
    if(inBlocks == NULL)
    {
        //TODO: 异常处理
        return;
    }
    if(inBlocks->size() == 0)
    {
        //TODO: 异常处理
        return;
    }

    if(inBlocks->size() == 1)
        heffman->statisticFreq(0, inBlocks->at(0));
    else
    {
        //多线程（需要阻塞主线程）
        std::vector<std::future<void>> results;
        std::vector<PTaskT> tasks;
        checkTpool();
        for(int i = 0; i < inBlocks->size(); ++i)
        {
            auto task = genTask(i);
            tasks.push_back(task);
            results.push_back(task->get_future());
            tpool->addTask(std::to_string(i), [task]() { (*task)(); });
        }
        for(auto& result : results)
        {
            result.get();
        }
    }
}

void GetFreq::checkTpool()
{
    int threadNums = tpool->getThreadNums();
    if(inBlocks->size() == threadNums)
        return;
    else if(inBlocks->size() < threadNums)
    {
        for(int i = inBlocks->size(); i < threadNums; ++i)
        {
            tpool->delThread(std::to_string(i));
        }
    }
    else
    {
        for(int i = threadNums; i < inBlocks->size(); ++i)
        {
            tpool->newThread(std::to_string(i));
        }
    }
}

void GetFreq::work(const int& i)
{
    heffman->statisticFreq(i, inBlocks->at(i));
}

GetFreq::PTaskT GetFreq::genTask(const int& i)
{
    std::packaged_task<void()> *task_ptr = new std::packaged_task<void()>(
        [this, i] { this->work(i); }
    );
    PTaskT task(task_ptr);
    return task;
}
