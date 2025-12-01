#include "DataBlocks.h"

DataBlocks::DataBlocks()
{ }

DataBlocks::~DataBlocks()
{ }

const sfc::blocks_t* DataBlocks::get_input_blocks()
{
    //TODO: 需要轮转first与second(函数)
    return this;
}

sfc::blocks_t* DataBlocks:: get_output_blocks()
{

    return this;
}

//TODO: 需要size()方法返回有效块的个数
int DataBlocks::size()
{

}