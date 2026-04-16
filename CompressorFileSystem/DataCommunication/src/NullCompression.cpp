#include "../include/NullCompression.h"
#include <cstring>

namespace Y_flib
{

void NullCompression::statistic_freq(const DataBlock &data)
{
    m_lastBlockSize = data.size();
}

void NullCompression::build_encode_tree()
{
    // no-op
}

void NullCompression::serialize_tree(DataBlock &outTree)
{
    outTree.clear();
    outTree.resize(1 + sizeof(uint64_t));
    outTree[0] = NULL_TREE_MARKER;
    uint64_t size = static_cast<uint64_t>(m_lastBlockSize);
    std::memcpy(outTree.data() + 1, &size, sizeof(uint64_t));
}

void NullCompression::deserialize_tree(const DataBlock &inTree)
{
    if (inTree.size() < 1 + sizeof(uint64_t) || inTree[0] != NULL_TREE_MARKER)
        return;
    uint64_t size = 0;
    std::memcpy(&size, inTree.data() + 1, sizeof(uint64_t));
    m_lastBlockSize = static_cast<size_t>(size);
}

void NullCompression::encode(const DataBlock &input, DataBlock &output)
{
    output = input;
}

void NullCompression::decode(const DataBlock &input, DataBlock &output, size_t originalSize)
{
    size_t copySize = (originalSize < input.size()) ? originalSize : input.size();
    output.assign(input.begin(), input.begin() + copySize);
}

}
