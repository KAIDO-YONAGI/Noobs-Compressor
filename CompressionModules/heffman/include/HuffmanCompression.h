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
        explicit HuffmanCompression(int threadNums = 1) : huffman(std::make_unique<Huffman>(threadNums)) {}

        void compress(const DataBlock &input, DataBlock &metadataOut, DataBlock &output) override
        {
            huffman->statisticFreq(0, input);
            huffman->mergeTtabs();
            huffman->genHefftree();
            huffman->saveCodeInTab();
            huffman->treeToPlatUchar(metadataOut);
            huffman->encode(input, output);
        }

        void decompress(const DataBlock &metadata, const DataBlock &input, DataBlock &output, size_t originalSize) override
        {
            huffman->spawnTree(const_cast<DataBlock &>(metadata));
            huffman->decode(input, output, BitHandler(), originalSize);
        }

        Huffman *getHuffman() { return huffman.get(); }

    private:
        std::unique_ptr<Huffman> huffman;
    };

} // namespace Y_flib
