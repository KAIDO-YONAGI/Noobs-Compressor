#pragma once

#include "FileLibrary.h"
#include "ICompression.h"
#include "IEncryption.h"
#include "NullCompression.h"
#include "NullEncryption.h"
#include <memory>
#include <stdexcept>
#include <string>

namespace Y_flib
{
    struct StrategyModules
    {
        std::unique_ptr<ICompression> compression;
        std::unique_ptr<IEncryption> encryption;
    };

    class StrategyFactory
    {
    public:
        static StrategyModules createModules(CompressionMode mode, const std::string &password);

        /*    enum class CompressionMode : uint8_t
            {
                HuffmanAES  = 0, // 默认：Huffman压缩 + AES加密（向后兼容）
                HuffmanOnly = 1, // 仅Huffman压缩，无加密
                AESOnly     = 2, // 仅AES加密，无压缩
                PackOnly    = 3  // 仅打包，无压缩无加密
            };
        */
        static CompressionMode idToMode(CompressStrategy id)
        {
            // id 来自归档文件头，属不可信输入，穷举校验通过后才映射回枚举。
            // switch 刻意对枚举本身做且不带 default：新增 CompressionMode 枚举值
            // 时由 -Wswitch 在编译期提醒来此补 case；未知 id 落到下方的 throw。
            switch (static_cast<CompressionMode>(id))
            {
            case CompressionMode::HuffmanAES:
            case CompressionMode::HuffmanOnly:
            case CompressionMode::AESOnly:
            case CompressionMode::PackOnly:
                return static_cast<CompressionMode>(id);
            }
            throw std::runtime_error("Unsupported compression strategy: " + std::to_string(id));
        }

        static CompressStrategy modeToId(CompressionMode mode)
        {
            return static_cast<CompressStrategy>(mode);
        }

        static bool hasEncryption(CompressionMode mode)
        {
            return mode == CompressionMode::HuffmanAES || mode == CompressionMode::AESOnly;
        }
    };
}
