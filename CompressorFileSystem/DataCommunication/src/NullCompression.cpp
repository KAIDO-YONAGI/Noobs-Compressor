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

void NullCompression::decompress(const DataBlock &metadata, const DataBlock &input, DataBlock &output, size_t originalSize)
{
    // 透传数据，按 originalSize 截断（处理最后一个不完整块）
    size_t copySize = (originalSize < input.size()) ? originalSize : input.size();
    output.assign(input.begin(), input.begin() + copySize);
}

}
