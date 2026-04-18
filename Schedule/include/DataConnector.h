#ifndef DATACONNECTOR_H_INTERFACE
#define DATACONNECTOR_H_INTERFACE

// 前向声明
namespace sfc {
    class DataBlocks;
    using blocks_t = DataBlocks;
}

/**
 * 用于类型擦除的基类
 * 当需要传输特定类型的数据结构时，可以使用继承该类的派生类
 * 包装数据结构，存入继承了DataConnector的数据块类中。
 */
/*
 struct Type
{
    virtual ~Type() = default;
};
*/

/**
 * 数据传输接口
 * 有两个分别负责返回输入块列表与输出块列表的
 * 纯虚函数。数据块类应该继承这个抽象类并实现
 * 相应方法。以及两个用于特定数据结构传输的接口
 *
 * getInputBlocks()：获取输入块列表的指针，只读
 * getOutputBlocks()：获取输出块列表的指针，可写
 * done()：算法模块完成对数据的读写后可调用，以善后或为
 *         后续工作作准备
 */
class DataConnector
{
public:
    virtual ~DataConnector() = default;

    virtual sfc::blocks_t* getInputBlocks() = 0;
    virtual sfc::blocks_t* getOutputBlocks() = 0;
    virtual void done() = 0;
    //virtual void ready_put_value() = 0;
    //virtual Type& get_value() = 0;
};

#endif //DATACONNECTOR_H_INTERFACE
