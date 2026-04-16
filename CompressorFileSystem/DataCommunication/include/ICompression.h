#pragma once

#include "FileLibrary.h"

namespace Y_flib
{
    /**
     * 压缩模块接口
     * 所有压缩算法实现都应继承此接口
     */
    class ICompression
    {
    public:
        virtual ~ICompression() = default;

        // 统计数据频率（压缩前调用）
        virtual void statistic_freq(const DataBlock &data) = 0;

        // 构建编码树
        virtual void build_encode_tree() = 0;

        // 序列化编码树到字节流
        virtual void serialize_tree(DataBlock &outTree) = 0;

        // 从字节流反序列化编码树
        virtual void deserialize_tree(const DataBlock &inTree) = 0;

        // 编码压缩
        virtual void encode(const DataBlock &input, DataBlock &output) = 0;

        // 解码解压
        virtual void decode(const DataBlock &input, DataBlock &output, size_t originalSize) = 0;
    };

} // namespace Y_flib
