#include "DataBlocksManage.h"
using namespace sfc;

DataBlocks::DataBlocks(int blockNums):
blocks(blockNums)
{ }

DataBlocks::~DataBlocks()
{ }

int DataBlocks::size()
{
    int result = 0;  // 初始化为0
    for(auto& block: blocks)
    {
        if(block.size() != 0)
            ++result;
    }
    return result; 
}

sfc::block_t& DataBlocks::at(int blockPosition)
{ 
    return blocks.at(blockPosition);
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

std::vector<block_t>::const_iterator DataBlocks::cbegin()
{
    return blocks.cbegin();
}

std::vector<sfc::block_t>::iterator DataBlocks::end()
{
    return blocks.end();
}

std::vector<block_t>::const_iterator DataBlocks::cend()
{
    return blocks.cend();
}

void DataBlocks::checkAndFix()
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


DataBlocksManage::DataBlocksManage(int blockNums):
whichOut(1), dataBlocksArray{DataBlocks(blockNums), DataBlocks(blockNums)}
{ }

DataBlocksManage::~DataBlocksManage()
{ }

sfc::blocks_t* DataBlocksManage::getInputBlocks()
{
    return &dataBlocksArray[1 - whichOut];
}

sfc::blocks_t* DataBlocksManage:: getOutputBlocks()
{
    return &dataBlocksArray[whichOut];
}

//FIXME: 模块算法完成后调用done
void DataBlocksManage::done()
{
    rotateIo();
    dataBlocksArray[1 - whichOut].clear();
    dataBlocksArray[whichOut].checkAndFix();
}

void DataBlocksManage::rotateIo()
{
    whichOut = (whichOut + 1) % 2;
}
