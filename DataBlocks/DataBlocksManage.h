#ifndef DATABLOCKSMANAGE_H
#define DATABLOCKSMANAGE_H
#include <string>
#include <cstdint>
#include <vector>
#include <algorithm>
#include "../Schedule/include/DataConnector.h"

/**
 * 数据块列表类
 * 含一组sfc::block_t
 * TODO: 可以决定是否返回const块
 */
namespace sfc
{
    using block_t = std::vector<unsigned char>;
    class DataBlocks
    {
    public:
        DataBlocks(int);
        ~DataBlocks();

        int size();
        block_t& at(int);
        void clear();
        std::vector<block_t>::iterator begin();
        std::vector<block_t>::const_iterator cbegin();
        std::vector<block_t>::iterator end();
        std::vector<block_t>::const_iterator cend();
        void checkAndFix();

    private:
        std::vector<block_t> blocks;
        
    };

    using blocks_t = DataBlocks;
}
/**
 * 比特流数据块列表管理者
 * 包含两个块列表，轮转in块和out块的属性
 * 
 */
class DataBlocksManage: public DataConnector
{
public:
    DataBlocksManage(int);
    ~DataBlocksManage();

    sfc::blocks_t* getInputBlocks() override;
    sfc::blocks_t* getOutputBlocks() override;
    void done() override;

private:
    sfc::DataBlocks dataBlocksArray[2];
    uint8_t whichOut;

    void rotateIo();
};


#endif //DATABLOCKSMANAGE_H
