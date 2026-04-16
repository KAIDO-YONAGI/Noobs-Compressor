#pragma once

#include "../../../CompressorFileSystem/DataCommunication/include/ICompression.h"
#include "../../hefftype/Heffman_type.h"
#include "Heffman.h"
#include <memory>

namespace Y_flib
{
    /**
     * Huffman 压缩实现
     * 包装 Heffman 类，实现 ICompression 接口
     */
    class HuffmanCompression : public ICompression
    {
    public:
        explicit HuffmanCompression(int mode = 1) : m_heffman(std::make_unique<Heffman>(mode)) {}

        void statistic_freq(const DataBlock &data) override
        {
            m_heffman->statistic_freq(0, data);
            m_heffman->merge_ttabs();
        }

        void build_encode_tree() override
        {
            m_heffman->gen_hefftree();
            m_heffman->save_code_inTab();
        }

        void serialize_tree(DataBlock &outTree) override
        {
            m_heffman->tree_to_plat_uchar(outTree);
        }

        void deserialize_tree(const DataBlock &inTree) override
        {
            m_heffman->spawn_tree(const_cast<DataBlock &>(inTree));
        }

        void encode(const DataBlock &input, DataBlock &output) override
        {
            m_heffman->encode(input, output);
        }

        void decode(const DataBlock &input, DataBlock &output, size_t originalSize) override
        {
            m_heffman->decode(input, output, BitHandler(), originalSize);
        }

        // 获取底层 Heffman 对象（用于特殊操作）
        Heffman *get_heffman() { return m_heffman.get(); }

    private:
        std::unique_ptr<Heffman> m_heffman;
    };

} // namespace Y_flib
