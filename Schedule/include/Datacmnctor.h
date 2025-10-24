#ifndef DATACOMUNICATOR_H_INTERFACE
#define DATACOMUNICATOR_H_INTERFACE

#include "../../namespace/namespace_sfc.h"

/**
 * 数据传输接口
 * 有两个分别负责返回输入块列表与输出块列表的
 * 纯虚函数。数据块类应该继承这个抽象类并实现
 * 相应方法。
 * 现在，伊玛，
 * 该接口可能作为一个指针传给Worker接口的work函数，
 * 也可能在策略接口的实现内就取出数据块，将它们传递给
 * work函数。
 */

class Datacmnctor 
{
public:
    virtual ~Datacmnctor() = default;

private:

public:
    virtual sfc::blocks_t* get_input_blocks() = 0;
    virtual sfc::blocks_t* get_output_blocks() = 0;
};

#endif //DATACOMUNICATOR_H_INTERFACE