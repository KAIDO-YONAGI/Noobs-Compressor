#pragma once

#include "../../../CompressorFileSystem/DataCommunication/include/IEncryption.h"
#include "My_Aes.h"
#include <memory>

namespace Y_flib
{
    /**
     * AES 加密实现
     * 包装 Aes 类，实现 IEncryption 接口
     */
    class AesEncryption : public IEncryption
    {
    public:
        explicit AesEncryption(Aes *aes) : aes(aes) {}

        void encrypt(const DataBlock &input, DataBlock &output) override
        {
            output.clear();
            output.resize(input.size());  // AES-CFB 无填充
            aes->doAes(1, input, output);
        }

        void decrypt(const DataBlock &input, DataBlock &output) override
        {
            output.clear();
            output.resize(input.size());  // AES-CFB 无填充
            aes->doAes(2, input, output);
        }

        // 获取底层 Aes 对象
        Aes *getAes() { return aes; }

    private:
        Aes *aes;
    };

} // namespace Y_flib
