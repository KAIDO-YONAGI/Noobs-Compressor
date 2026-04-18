#ifndef DOENCODE_H
#define DOENCODE_H

#include <future>
#include <memory>
#include "Heffman.h"
#include "../../Schedule/include/Worker.h"
#include "../../../ThreadPool/ThreadPool.h"

/**
 * 赫夫曼进行压缩的功能模块
 * 类内组合的heffman在堆上唯一
 * 
 * 实现Worker接口，可供调度器调用
 * 
 *    用户接口：
 *    work(DataConnector*)：
 *        将数据块指针保存在类内
 *
 *    私有：
 *    checkTpool()：
 *        检查是否要创建/销毁线程
 *    work(int)：
 *        实际操作，调用编码功能
 *    genTask(int):
 *        生成packaged_task包装的work(int)任务函数，传给线程池
 */
//TODO: 完成拷贝操作/移动操作
class DoEncode: public Worker
{
public:
    DoEncode(Huffman*);
    ~DoEncode();

    void work(DataConnector*) override;

private:
    using PTaskT = std::shared_ptr<std::packaged_task<void()>>;
    std::shared_ptr<Huffman> heffman;
    std::unique_ptr<ThreadPool> tpool;
    sfc::blocks_t *inBlocks;
    sfc::blocks_t *outBlocks;

    void checkTpool();
    void work(const int&);
    PTaskT genTask(const int&);

};


#endif //DOENCODE_H