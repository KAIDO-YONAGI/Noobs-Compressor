#ifndef NAMESPACE_SFC_H
#define NAMESPACE_SFC_H

#include <vector>
#include <queue>
#include <functional>

//TODO：后续实际实现的blocks_t，需要size()方法返回有效块的个数

/**
 * 命名空间
 * 自定义类型：
 *     byte：数据处理流基本单位（比如文件--压缩--加密，是byte流）
 *     block_t：一个数据块类型，模块算法中使用该类型进行io。
 *     blocks_t：一个数据块列表，用于模块数据交互。应该以引用的
 *              方式取出块。
 */
namespace sfc{
    using byte = unsigned char;
    using block_t = std::vector<unsigned char>;
    using blocks_t = std::vector< block_t >;

    using task_queue = std::queue< std::function<void()> >;
}

#endif //NAMESPACE_SFC_H