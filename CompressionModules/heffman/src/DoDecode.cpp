#include "../include/DoDecode.h"

DoDecode::DoDecode(Huffman* heffcore):
    heffman(heffcore), inBlocks(NULL), outBlocks(NULL)
{ }

DoDecode::~DoDecode()
{ }

void DoDecode::work(DataConnector* datacmnctor)
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
    auto iter_inbs = inBlocks->cbegin();
    auto iter_outbs = outBlocks->begin();
    while(iter_inbs != inBlocks->cend())
    {
        heffman->decode(*iter_inbs, *iter_outbs);
        ++iter_outbs; //TODO: 需要在缓冲块类中保证out数量严格等于in
        if(iter_outbs == outBlocks->end())
        {
            //TODO: 输出块不足异常，与日志
            return;
        }
    }
}
