#include "DataBlocksManage.h"
using namespace sfc;

DataBlocks::DataBlocks(int blocks_nums):
blocks(blocks_nums)
{ }

DataBlocks::~DataBlocks()
{ }

int DataBlocks::size()
{
    int result;
    for(auto& block: blocks)
    {
        if(block.size() != 0)
            ++result;
    }
    return result;
}

sfc::block_t& DataBlocks::at(int pos)
{
    return blocks.at(pos);
}

void DataBlocks::clear()
{
    for(auto& block: blocks)
    {
        block.clear();
    }
}

std::vector<sfc::block_t>::iterator DataBlocks::begin()
{
    return blocks.begin();
}

std::vector<sfc::block_t>::iterator DataBlocks::end()
{
    return blocks.end();
}

void DataBlocks::check_and_fix()
{
    auto it = std::remove_if(
        blocks.begin(),
        blocks.end(),
        [] (sfc::block_t& block) { 
            if(block.size() == 0)
                return true;
            else
                return false;
        }
    );
    blocks.erase(it, blocks.end());
}


DataBlocksManage::DataBlocksManage(int blocks_nums):
which_out(1), blockss{DataBlocks(blocks_nums), DataBlocks(blocks_nums)}
{ }

DataBlocksManage::~DataBlocksManage()
{ }

sfc::blocks_t* DataBlocksManage::get_input_blocks()
{
    return &blockss[1 - which_out];
}

sfc::blocks_t* DataBlocksManage:: get_output_blocks()
{
    return &blockss[which_out];
}

//FIXME: 模块算法完成后调用done
void DataBlocksManage::done()
{
    rotate_io();
    blockss[1 - which_out].clear();
    blockss[which_out].check_and_fix();
}

void DataBlocksManage::rotate_io()
{
    which_out = (which_out + 1) % 2;
}
