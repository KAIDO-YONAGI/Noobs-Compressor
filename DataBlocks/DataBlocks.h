#ifndef DATABLOCKS_H
#define DATABLOCKS_H

#include <vector>
#include "../Schedule/include/Datacmnctor.h"

class DataBlocks: public Datacmnctor
{
public:
    DataBlocks();
    ~DataBlocks();

    const sfc::blocks_t* get_input_blocks() override;    
    sfc::blocks_t* get_output_blocks() override;
    int size();

private:
    std::vector<sfc::block_t> first_blocks;
    std::vector<sfc::block_t> second_blocks;

};

#endif //DATABLOCKS_H