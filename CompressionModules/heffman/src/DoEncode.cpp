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
        
    }
}

void DoEncode::check_tpool()
{
    for(int i = 0; i < in_blocks->size(); ++i)
    {
        
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
