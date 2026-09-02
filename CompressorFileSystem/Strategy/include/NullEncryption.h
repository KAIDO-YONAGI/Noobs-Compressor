#pragma once

#include "IEncryption.h"

namespace Y_flib
{
    class NullEncryption : public IEncryption
    {
    public:
        NullEncryption() = default;

        void encrypt(const DataBlock &input, DataBlock &output) override
        {
            output = input;
        }

        void decrypt(const DataBlock &input, DataBlock &output) override
        {
            output = input;
        }
    };
}
