#pragma once

#include "FileLibrary.h"

namespace Y_flib
{
    /**
     * 压缩模块接口（算法无关）
     *
     * 所有压缩算法只需实现 compress 和 decompress 两个方法。
     * 调用方（CompressionLoop / DecompressionLoop）通过这两个方法完成数据块的压缩与解压，
     * 无需了解算法内部的步骤（如建树、频率统计等）。
     *
     * metadata 用于存储算法在解压时需要还原的附带信息，内容由算法自行定义，对调用方透明：
     *   - Huffman 算法：存储序列化的编码树
     *   - Null 算法：存储标记字节 + 原始数据大小（占 9 字节）
     *   - 未来算法：字典、滑动窗口状态等
     * metadata 在压缩时由 compress() 输出，经加密后写入归档文件；
     * 解压时从归档文件读出，经解密后传入 decompress()。
     *
     * 压缩侧调用流程（CompressionLoop）：
     *   compress(input, metadata, compressedData)
     *   → encrypt(metadata) → write to archive
     *   → encrypt(compressedData) → write to archive
     *
     * 解压侧调用流程（DecompressionLoop）：
     *   read metadata block → decrypt → decompress(metadata, compressedData, output, originalSize)
     */
    class ICompression
    {
    public:
        virtual ~ICompression() = default;

        /**
         * 压缩一个数据块
         * @param input       原始数据块（通常为 BUFFER_SIZE = 8MB）
         * @param metadataOut 输出：算法在解压时需要的附带信息（如编码树），由调用方加密后写入归档
         * @param output      输出：压缩后的数据，由调用方加密后写入归档
         */
        virtual void compress(const DataBlock &input, DataBlock &metadataOut, DataBlock &output) = 0;

        /**
         * 解压一个数据块
         * @param metadata     压缩时由 compress() 输出、经解密后的附带信息
         * @param input        经解密后的压缩数据
         * @param output       输出：解压后的数据
         * @param originalSize 期望的解压字节数（用于截断最后一个不完整块）
         */
        virtual void decompress(const DataBlock &metadata, const DataBlock &input, DataBlock &output, size_t originalSize) = 0;
    };

} // namespace Y_flib
