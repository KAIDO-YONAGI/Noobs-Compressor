#pragma once

#include "FileLibrary.h"
#include "ICompression.h"
#include "IEncryption.h"
#include "NullCompression.h"
#include "NullEncryption.h"
#include <memory>
#include <stdexcept>
#include <string>

class Aes;

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
            // 文件头里持久化的策略号就是 CompressionMode 的底层值（见 modeToId），
            // 因此这里只校验 id 是否为已知模式，再按底层值映射回枚举；
            // 不使用 0/1/2/3 魔法数字，枚举定义是唯一事实来源。
            switch (id)
            {
            case static_cast<CompressStrategy>(CompressionMode::HuffmanAES):
            case static_cast<CompressStrategy>(CompressionMode::HuffmanOnly):
            case static_cast<CompressStrategy>(CompressionMode::AESOnly):
            case static_cast<CompressStrategy>(CompressionMode::PackOnly):
                return static_cast<CompressionMode>(id);
            default:
                throw std::runtime_error("Unsupported compression strategy: " + std::to_string(id));
            }
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
