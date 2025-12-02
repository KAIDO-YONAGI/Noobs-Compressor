#ifndef DATABLOCKSMANAGE_H
#define DATABLOCKSMANAGE_H

#include <cstdint>
#include <vector>
#include <algorithm>
#include "../Schedule/include/Datacmnctor.h"

/**
 * 数据块列表类
 * 含一组sfc::block_t
 * TODO: 可以决定是否返回const块
 */
class DataBlocks
{
public:
    DataBlocks(int);
    ~DataBlocks();

    int size();
    sfc::block_t& at(int);
    void clear();
    std::vector<sfc::block_t>::iterator begin();
    std::vector<sfc::block_t>::iterator end();
    void check_and_fix();

private:
    std::vector<sfc::block_t> blocks;
    
};

/**
 * 比特流数据块列表管理者
 * 包含两个块列表，轮转in块和out块的属性
 * 
 */
class DataBlocksManage: public Datacmnctor
{
public:
    DataBlocksManage(int);
    ~DataBlocksManage();

    sfc::blocks_t* get_input_blocks() override;    
    sfc::blocks_t* get_output_blocks() override;
    void done() override;

private:
    DataBlocks blockss[2];
    uint8_t which_out;

    void rotate_io();
};


#endif //DATABLOCKSMANAGE_H