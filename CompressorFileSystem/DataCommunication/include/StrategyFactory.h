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

        static CompressionMode idToMode(CompressStrategy id)
        {
            switch (id)
            {
            case 0: return CompressionMode::HuffmanAES;
            case 1: return CompressionMode::HuffmanOnly;
            case 2: return CompressionMode::AESOnly;
            case 3: return CompressionMode::PackOnly;
            default: throw std::runtime_error("Unsupported compression strategy: " + std::to_string(id));
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
