#pragma once

#include "FileLibrary.h"

namespace Y_flib
{
    /**
     * 加密模块接口
     * 所有加密算法实现都应继承此接口
     */
    class IEncryption
    {
    public:
        virtual ~IEncryption() = default;

        // 加密
        virtual void encrypt(const DataBlock &input, DataBlock &output) = 0;

        // 解密
        virtual void decrypt(const DataBlock &input, DataBlock &output) = 0;
    };

} // namespace Y_flib
