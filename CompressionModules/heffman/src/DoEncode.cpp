#include "../include/DoEncode.h"

DoEncode::DoEncode(Huffman* heffcore):
heffman(heffcore), inBlocks(NULL), outBlocks(NULL),
tpool(new ThreadPool())
{ }

DoEncode::~DoEncode()
{ }

void DoEncode::work(DataConnector* datacmnctor)
{
    inBlocks = datacmnctor->getInputBlocks();
    outBlocks = datacmnctor->getOutputBlocks();
    if(inBlocks == NULL)
    {
        //TODO: 异常处理
        return;
    }
    if(outBlocks == NULL)
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
        heffman->encode(inBlocks->at(0), outBlocks->at(0));
    else
    {
        std::vector<std::future<void>> results;
        std::vector<PTaskT> tasks;
        checkTpool();
        for(int i = 0; i < inBlocks->size(); ++i)
        {
            auto task = genTask(i);
            tasks.push_back(task);
            results.push_back(task->get_future());
            // 创建一个可调用的包装器
            tpool->addTask(std::to_string(i), [task]() { (*task)(); });
        }
        for(auto& result: results)
        {
            result.get();
        }
    }
}

void DoEncode::checkTpool()
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

void DoEncode::work(const int& thread_id)
{
    heffman->encode(inBlocks->at(thread_id), outBlocks->at(thread_id));
}

DoEncode::PTaskT DoEncode::genTask(const int& thread_id)
{
    std::packaged_task<void()> *task_ptr = new std::packaged_task<void()>(
        [this, thread_id] { this->work(thread_id); }
    );
    PTaskT task(task_ptr);
    return task;
}
