#pragma once

#include "ICompression.h"
#include <cstdint>

namespace Y_flib
{
    class NullCompression : public ICompression
    {
    public:
        NullCompression() = default;

        void statistic_freq(const DataBlock &data) override;
        void build_encode_tree() override;
        void serialize_tree(DataBlock &outTree) override;
        void deserialize_tree(const DataBlock &inTree) override;
        void encode(const DataBlock &input, DataBlock &output) override;
        void decode(const DataBlock &input, DataBlock &output, size_t originalSize) override;

    private:
        static constexpr uint8_t NULL_TREE_MARKER = 0x00;
        size_t m_lastBlockSize = 0;
    };
}
