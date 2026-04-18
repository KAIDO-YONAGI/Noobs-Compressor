#ifndef DODECODE_H
#define DODECODE_H

#include <memory>
#include "Heffman.h"
#include "../../Schedule/include/Worker.h"
#include "../../../ThreadPool/ThreadPool.h"
#include "../../../Schedule/include/Worker.h"

/**
 * 赫夫曼的解压缩功能模块
 * 类内组合的heffman在堆上唯一
 * 不支持多线程加速！
 */
class DoDecode: public Worker
{
public:
    DoDecode(Huffman*);
    ~DoDecode();

    void work(DataConnector*) override;

private:
    std::shared_ptr<Huffman> heffman;
    sfc::blocks_t* inBlocks;
    sfc::blocks_t* outBlocks;

};


#endif //DODECODE_H
