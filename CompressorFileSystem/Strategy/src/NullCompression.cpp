#include "../include/NullCompression.h"
#include <cstring>

namespace Y_flib
{

    void NullCompression::compress(const DataBlock &input, DataBlock &metadataOut, DataBlock &output)
    {
        // 写入 9 字节 metadata：标记字节 + 原始数据大小
        metadataOut.clear();
        metadataOut.resize(1 + sizeof(FileSize));
        metadataOut[0] = NULL_COMPRESSION_MARKER;
        FileSize size = static_cast<FileSize>(input.size());
        std::memcpy(metadataOut.data() + 1, &size, sizeof(FileSize));

        output = input;
    }

    // metadata 是压缩侧写入的"标记 + 原始大小"（见上方 compress），透传模式不需要它，
    // 但它是 ICompression 接口签名的一部分，故显式标注未使用而非省略参数名
    void NullCompression::decompress([[maybe_unused]] const DataBlock &metadata, const DataBlock &input, DataBlock &output, size_t originalSize)
    {
        // 透传数据，按 originalSize 截断（处理最后一个不完整块）
        size_t copySize = (originalSize < input.size()) ? originalSize : input.size();
        output.assign(input.begin(), input.begin() + copySize);
    }

}
