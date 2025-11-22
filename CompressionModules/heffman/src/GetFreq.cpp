#include "../include/GetFreq.h"

inline GetFreq::GetFreq(Heffman* heffcore):
    heffman(heffcore), tpool(new ThreadPool())
{ }

inline GetFreq::~GetFreq()
{ }

void GetFreq::work(Datacmnctor *datacmnctor)
{
    in_blocks = datacmnctor->get_input_blocks();
    if(in_blocks == NULL)
    {
        //TODO: 异常处理
        return;
    }
    if(in_blocks->size() == 0)
    {
        //TODO: 异常处理
        return;
    }

    if(in_blocks->size() == 1)
        heffman->statistic_freq(0, in_blocks->at(0));
    else 
    {
        //多线程（需要阻塞主线程）
        std::vector<std::future<void>> results;
        std::vector<ptask_t> tasks;
        check_tpool();
        for(int i = 0; i < in_blocks->size(); ++i)
        {
            auto task = gen_task(i);
            tasks.push_back(task);
            results.push_back(task->get_future());
            tpool->add_task(std::to_string(i), task);
        }
        for(auto& result : results)
        {
            result.get();
        }
    }
}

void GetFreq::check_tpool()
{
    int thread_nums = tpool->get_thread_nums();
    if(in_blocks->size() == thread_nums)
        return;
    else if(in_blocks->size() < thread_nums)
    {
        for(int i = in_blocks->size(); i < thread_nums; ++i)
        {
            tpool->del_thread(std::to_string(i));
        }
    }
    else
    {
        for(int i = thread_nums; i < in_blocks->size(); ++i)
        {
            tpool->new_thread(std::to_string(i));
        }
    }
}

void GetFreq::work(const int& i)
{
    heffman->statistic_freq(i, in_blocks->at(i)); 
}

GetFreq::ptask_t GetFreq::gen_task(const int& i)
{
    std::packaged_task<void()> *task_ptr = new std::packaged_task<void()>(
        [this, i] { this->work(i); }
    );
    ptask_t task(task_ptr);
    return task;
}
