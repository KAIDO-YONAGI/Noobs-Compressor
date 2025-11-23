#ifndef DATACOMUNICATOR_H_INTERFACE
#define DATACOMUNICATOR_H_INTERFACE

#include "../../namespace/namespace_sfc.h"

/**
 * 用于类型擦除的基类
 * 当需要传输特定类型的数据结构时，可以使用继承该类的派生类
 * 包装数据结构，存入继承了Datacmnctor的数据块类中。
 */
struct Type
{
    virtual ~Type() = default;
};

/**
 * 数据传输接口
 * 有两个分别负责返回输入块列表与输出块列表的
 * 纯虚函数。数据块类应该继承这个抽象类并实现
 * 相应方法。以及两个用于特定数据结构传输的接口
 * 
 * get_input_blocks()：获取输入块列表的指针，只读
 * get_output_blocks()：获取输出块列表的指针，可写
 * ready_put_value()：某模块准备输出时调用
 * get_value()：获取擦除类型，转型后可得到数据结构的指针，作读或写
 */
class Datacmnctor 
{
public:
    virtual ~Datacmnctor() = default;

    //FIXME: get_input_blocks返回值改为只读
    virtual sfc::blocks_t* get_input_blocks() = 0;
    virtual sfc::blocks_t* get_output_blocks() = 0;
    virtual void ready_put_value() = 0;
    virtual Type& get_value() = 0;
};

#endif //DATACOMUNICATOR_H_INTERFACE