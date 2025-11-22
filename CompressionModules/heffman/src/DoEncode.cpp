#include "../include/DoEncode.h"

DoEncode::DoEncode(/* args */)
{
}

DoEncode::~DoEncode()
{
}

void DoEncode::work(Datacmnctor* datacmnctor)
{
    in_blocks = datacmnctor->get_input_blocks();
    out_blocks = datacmnctor->get_output_blocks();
    if(in_blocks->size() == 1)
        heffman->encode(0, in_blocks->at(0), out_blocks->at(0));
    else
    {
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
        for(auto& result: results)
        {
            result.get();
        }
    }
}

void DoEncode::check_tpool()
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

void DoEncode::work(const int& thread_id)
{
    heffman->encode(thread_id, in_blocks->at(thread_id), out_blocks->at(thread_id));
}

DoEncode::ptask_t DoEncode::gen_task(const int& thread_id)
{
    std::packaged_task<void()> *task_ptr = new std::packaged_task<void()>(
        [this, thread_id] { this->work(thread_id); }
    );
    ptask_t task(task_ptr);
    return task;
}
