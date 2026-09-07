#include "../include/My_Aes.h"

#pragma comment(lib, "advapi32.lib")

/* CFB 封装层：IV 生成/解析、密钥流生成与异或。块核心轮函数在 AesFunctions.cpp */

Y_flib::DataBlock Aes::processDataAes(const Y_flib::DataBlock &inputBuffer, Y_flib::AesMode mode)
{
    Y_flib::DataBlock outputBuffer;
    uint8_t *body = nullptr;
    size_t bodyLen = 0;

    // 处理IV
    if (mode == Y_flib::AesMode::Encrypt)
    { // 加密
        // 生成随机IV (使用 Windows CryptoAPI)。
        // CSP 句柄获取是重操作（底层 RPC），逐次获取/释放在多工人并发下互相争抢，
        // 是流水线病态慢的根源——句柄缓存进实例，只获取一次，IV 仍每次随机
        if (cryptProvider == 0)
        {
            // 尝试多种提供商类型以提高兼容性
            DWORD providers[] = {
                PROV_RSA_AES,    // Windows XP SP3+
                PROV_RSA_FULL    // 旧版Windows
            };
            for (DWORD provType : providers)
            {
                if (CryptAcquireContext(&cryptProvider, NULL, NULL, provType,
                    CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
                    break;
            }
        }

        if (cryptProvider == 0 || !CryptGenRandom(cryptProvider, sizeof(iv), iv)) {
            throw std::runtime_error("Failed to generate random IV");
        }

        // 一次分配：前置 IV，随后原地把明文加密进输出缓冲，省去中间整块拷贝
        outputBuffer.resize(sizeof(iv) + inputBuffer.size());
        memcpy(outputBuffer.data(), iv, sizeof(iv));
        if (!inputBuffer.empty()) {
            memcpy(outputBuffer.data() + sizeof(iv), inputBuffer.data(), inputBuffer.size());
        }
        body = outputBuffer.data() + sizeof(iv);
        bodyLen = inputBuffer.size();
    }
    else if (mode == Y_flib::AesMode::Decrypt)
    { // 解密
        // 检查输入是否足够包含IV
        if (inputBuffer.size() < sizeof(iv))
        {
            throw std::runtime_error("Input too short to contain IV");
        }

        // 提取IV
        memcpy(iv, inputBuffer.data(), sizeof(iv));

        // 密文体拷入输出后原地解密（CFB 解密与加密共用同一条正向轮函数路径）
        bodyLen = inputBuffer.size() - sizeof(iv);
        outputBuffer.resize(bodyLen);
        if (bodyLen > 0) {
            memcpy(outputBuffer.data(), inputBuffer.data() + sizeof(iv), bodyLen);
        }
        body = outputBuffer.data();
    }

    // 原地处理密文体
    if (bodyLen > 0)
    {
        if (mode == Y_flib::AesMode::Encrypt)
            aes(reinterpret_cast<char *>(body), static_cast<int>(bodyLen)); // 加密
        else
            deAes(reinterpret_cast<char *>(body), static_cast<int>(bodyLen)); // 解密
    }

    return outputBuffer;
}
void Aes::doAes(Y_flib::AesMode mode, const Y_flib::DataBlock &inputBuffer, Y_flib::DataBlock &outputBuffer)
{

    // 枚举值只能经 static_cast 越界，仍保留穷举校验兜底
    if (mode != Y_flib::AesMode::Encrypt && mode != Y_flib::AesMode::Decrypt)
    {
        throw std::invalid_argument("Invalid AesMode. Use AesMode::Encrypt or AesMode::Decrypt.");
    }

    try
    {
        outputBuffer=processDataAes(inputBuffer, mode);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("AES processing failed: ") + e.what());
    }

    return;
}
void Aes::aes(char *p, int plen)
{
    if (!p || plen <= 0)
        return;

    uint8_t feedback[16];
    memcpy(feedback, iv, 16);

    for (int offset = 0; offset < plen;)
    {
        int blockSize = (plen - offset < 16) ? (plen - offset) : 16;

        // 加密反馈寄存器
        int tempArray[4][4];
        convertToIntArray(reinterpret_cast<char *>(feedback), tempArray);

        // AES加密轮次
        addRoundKey(tempArray, 0);
        for (int round = 1; round < 10; ++round)
        {
            subBytes(tempArray);
            shiftRows(tempArray);
            mixColumns(tempArray);
            addRoundKey(tempArray, round);
        }
        subBytes(tempArray);
        shiftRows(tempArray);
        addRoundKey(tempArray, 10);

        // 获取加密结果
        char encryptedFeedback[16];
        convertArrayToStr(tempArray, encryptedFeedback);

        // 执行XOR和更新反馈
        for (int i = 0; i < blockSize; ++i)
        {
            p[offset + i] ^= encryptedFeedback[i];
            feedback[i] = p[offset + i];
        }

        offset += blockSize;
    }

    // 安全清除
    SecureZeroMemory(feedback, sizeof(feedback));
}

void Aes::deAes(char *c, int clen)
{
    if (!c || clen <= 0)
        return;

    uint8_t feedback[16];
    memcpy(feedback, iv, 16);

    for (int offset = 0; offset < clen;)
    {
        int blockSize = (clen - offset < 16) ? (clen - offset) : 16;
        uint8_t cipherBackup[16] = {0};
        memcpy(cipherBackup, c + offset, blockSize);

        // 加密反馈寄存器
        int tempArray[4][4];
        convertToIntArray(reinterpret_cast<char *>(feedback), tempArray);

        // AES加密轮次
        addRoundKey(tempArray, 0);
        for (int round = 1; round < 10; ++round)
        {
            subBytes(tempArray);
            shiftRows(tempArray);
            mixColumns(tempArray);
            addRoundKey(tempArray, round);
        }
        subBytes(tempArray);
        shiftRows(tempArray);
        addRoundKey(tempArray, 10);

        // 获取加密结果
        char encryptedFeedback[16];
        convertArrayToStr(tempArray, encryptedFeedback);

        // 执行XOR
        for (int i = 0; i < blockSize; ++i)
        {
            c[offset + i] ^= encryptedFeedback[i];
        }

        // 更新反馈
        memcpy(feedback, cipherBackup, blockSize);
        offset += blockSize;
    }

    // 安全清除
    SecureZeroMemory(feedback, sizeof(feedback));
}
