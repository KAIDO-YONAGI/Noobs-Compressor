#include "../include/DoDecode.h"

DoDecode::DoDecode(Heffman* heffcore):
    heffman(heffcore), in_blocks(NULL), out_blocks(NULL)
{ }

DoDecode::~DoDecode()
{ }

void DoDecode::work(Datacmnctor* datacmnctor)
{
    in_blocks = datacmnctor->get_input_blocks();
    out_blocks = datacmnctor->get_output_blocks();

    if(in_blocks == NULL)
    {
        //TODO: 异常处理
        return;
    }
    if(out_blocks == NULL)
    {
        //TODO: 异常处理
        return;
    }
    if(in_blocks->size() == 0)
    {
        //TODO: 异常处理
        return;
    }
    auto iter_inbs = in_blocks->cbegin();
    auto iter_outbs = out_blocks->begin();
    while(iter_inbs != in_blocks->cend())
    {
        heffman->decode(*iter_inbs, *iter_outbs);
        ++iter_outbs; //TODO: 需要在缓冲块类中保证out数量严格等于in
        if(iter_outbs == out_blocks->end())
        {
            //TODO: 输出块不足异常，与日志
            return;
        }
    }
}
