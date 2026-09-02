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

        // 输出由 doAes 内部整体赋值，无需预分配：
        // 加密输出 = 16 字节 IV + 密文（比输入多 16）；解密输出 = 去掉 IV 头的明文
        void encrypt(const DataBlock &input, DataBlock &output) override
        {
            aes->doAes(1, input, output);
        }

        void decrypt(const DataBlock &input, DataBlock &output) override
        {
            aes->doAes(2, input, output);
        }

        // 获取底层 Aes 对象
        Aes *getAes() { return aes; }

    private:
        Aes *aes;
    };

} // namespace Y_flib
