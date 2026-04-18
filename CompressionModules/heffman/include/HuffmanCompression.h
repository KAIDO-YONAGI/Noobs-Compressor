#pragma once

#include "../../../CompressorFileSystem/DataCommunication/include/ICompression.h"
#include "../../hefftype/Heffman_type.h"
#include "Heffman.h"
#include <memory>

namespace Y_flib
{
    /**
     * Huffman 压缩的 ICompression 实现
     *
     * compress 内部流程：统计频率 → 合并频率表 → 生成编码树 → 生成编码表 → 序列化树(metadataOut) → 编码(output)
     * decompress 内部流程：从 metadata 反序列化编码树 → 解码
     */
    class HuffmanCompression : public ICompression
    {
    public:
        explicit HuffmanCompression(int thread_nums = 1) : m_heffman(std::make_unique<Heffman>(thread_nums)) {}

        void compress(const DataBlock &input, DataBlock &metadataOut, DataBlock &output) override
        {
            m_heffman->statistic_freq(0, input);
            m_heffman->merge_ttabs();
            m_heffman->gen_hefftree();
            m_heffman->save_code_inTab();
            m_heffman->tree_to_plat_uchar(metadataOut);
            m_heffman->encode(input, output);
        }

        void decompress(const DataBlock &metadata, const DataBlock &input, DataBlock &output, size_t originalSize) override
        {
            m_heffman->spawn_tree(const_cast<DataBlock &>(metadata));
            m_heffman->decode(input, output, BitHandler(), originalSize);
        }

        Heffman *get_heffman() { return m_heffman.get(); }

    private:
        std::unique_ptr<Heffman> m_heffman;
    };

} // namespace Y_flib
