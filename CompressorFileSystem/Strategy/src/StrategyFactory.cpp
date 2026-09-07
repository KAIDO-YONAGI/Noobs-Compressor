#include "../include/StrategyFactory.h"
#include "../../../CompressionModules/Huffman/Core/include/HuffmanCompression.h"
#include "../../../EncryptionModules/Aes/include/AesEncryption.h"

namespace Y_flib
{

    StrategyModules StrategyFactory::createModules(CompressionMode mode, const std::string &password)
    {
        StrategyModules modules;

        switch (mode)
        {
        case CompressionMode::HuffmanAES:
            modules.compression = std::make_unique<HuffmanCompression>();
            modules.encryption = std::make_unique<AesEncryption>(password);
            break;

        case CompressionMode::HuffmanOnly:
            modules.compression = std::make_unique<HuffmanCompression>();
            modules.encryption = std::make_unique<NullEncryption>();
            break;

        case CompressionMode::AESOnly:
            modules.compression = std::make_unique<NullCompression>();
            modules.encryption = std::make_unique<AesEncryption>(password);
            break;

        case CompressionMode::PackOnly:
            modules.compression = std::make_unique<NullCompression>();
            modules.encryption = std::make_unique<NullEncryption>();
            break;
        }

        return modules;
    }

}
