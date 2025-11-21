#ifndef WORKER_H_INTERFACE
#define WORKER_H_INTERFACE

/**
 * 将功能模块抽象为，可供调度器指挥的工人
 * 每个可调度模块都要继承该类
 * 现在，伊玛，
 * work函数可能接受数据交互接口，或者直接接受
 * 输入输出的两个数据块列表。
 */


#include "Datacmnctor.h"

class Worker
{
public:
    virtual ~Worker() = default;

public:
    virtual void work(Datacmnctor*) = 0;
};

Worker::Worker()
{

}

#endif //WORKER_H_INTERFACE