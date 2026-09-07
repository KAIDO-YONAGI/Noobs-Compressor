#pragma once

#include "ICompression.h"

namespace Y_flib
{
    /**
     * 透传压缩的 ICompression 实现（不压缩）
     *
     * metadata 格式（共 9 字节）：
     *   [0]     : 标记字节 NULL_COMPRESSION_MARKER = 0x00
     *   [1..8]  : 原始数据大小（uint64_t, little-endian）
     *
     * encode/decode 直接拷贝数据，metadata 仅记录原始大小以保持接口统一。
     */
    class NullCompression : public ICompression
    {
    public:
        NullCompression() = default;

        void compress(const DataBlock &input, DataBlock &metadataOut, DataBlock &output) override;
        void decompress(const DataBlock &metadata, const DataBlock &input, DataBlock &output, size_t originalSize) override;

    private:
        static constexpr MetadataMarker NULL_COMPRESSION_MARKER = 0x00;
    };
}
