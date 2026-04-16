#include "../include/StrategyFactory.h"
#include "../../../CompressionModules/heffman/include/HuffmanCompression.h"
#include "../../../EncryptionModules/Aes/include/AesEncryption.h"
#include "../../../EncryptionModules/Aes/include/My_Aes.h"

// AesEncryption持有裸指针Aes*但不拥有它，需要包装一层来管理Aes对象的生命周期
namespace
{
    class OwnedAesEncryption : public Y_flib::IEncryption
    {
        std::unique_ptr<Aes> m_ownedAes;
        Y_flib::AesEncryption m_impl;

    public:
        OwnedAesEncryption(std::unique_ptr<Aes> aes)
            : m_ownedAes(std::move(aes)), m_impl(m_ownedAes.get()) {}

        void encrypt(const Y_flib::DataBlock &input, Y_flib::DataBlock &output) override
        {
            m_impl.encrypt(input, output);
        }

        void decrypt(const Y_flib::DataBlock &input, Y_flib::DataBlock &output) override
        {
            m_impl.decrypt(input, output);
        }
    };
}

namespace Y_flib
{

StrategyModules StrategyFactory::createModules(CompressionMode mode, const std::string &password)
{
    StrategyModules modules;

    switch (mode)
    {
    case CompressionMode::HuffmanAES:
        modules.compression = std::make_unique<HuffmanCompression>(1);
        modules.encryption = std::make_unique<OwnedAesEncryption>(std::make_unique<Aes>(password.c_str()));
        break;

    case CompressionMode::HuffmanOnly:
        modules.compression = std::make_unique<HuffmanCompression>(1);
        modules.encryption = std::make_unique<NullEncryption>();
        break;

    case CompressionMode::AESOnly:
        modules.compression = std::make_unique<NullCompression>();
        modules.encryption = std::make_unique<OwnedAesEncryption>(std::make_unique<Aes>(password.c_str()));
        break;

    case CompressionMode::PackOnly:
        modules.compression = std::make_unique<NullCompression>();
        modules.encryption = std::make_unique<NullEncryption>();
        break;
    }

    return modules;
}

}
