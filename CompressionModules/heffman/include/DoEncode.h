#ifndef DOENCODE_H
#define DOENCODE_H

#include <future>
#include <memory>
#include "Heffman.h"
#include "../../../Schedule/include/Worker.h"
#include "../../../ThreadPool/ThreadPool.h"

/**
 * 赫夫曼进行压缩的功能模块
 * 类内组合的heffman在堆上唯一
 * 
 * 实现Worker接口，可供调度器调用
 * 
 *    用户接口：
 *    work(Datacmnctor*)：
 *        将数据块指针保存在类内
 */
//TODO: 完成拷贝操作/移动操作
class DoEncode: public Worker
{
public:
    DoEncode(Heffman*);
    ~DoEncode();

    void work(Datacmnctor*) override;

private:
    using ptask_t = std::shared_ptr<std::packaged_task<void()>>;
    std::shared_ptr<Heffman> heffman;
    std::unique_ptr<ThreadPool> tpool;
    sfc::blocks_t *in_blocks;
    sfc::blocks_t *out_blocks; 

    void check_tpool();
    void work(const int&);
    ptask_t gen_task(const int&);

};


#endif //DOENCODE_H