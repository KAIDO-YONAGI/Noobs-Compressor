#include "../include/My_Aes.h"

#include <stdexcept>
#include <string>

/* AES 底层原语层：密钥派生、CNG 密钥建立/销毁、CTR 密钥流生成。
 *
 * 与旧实现的分工保持一致：原 AesFunctions.cpp 放的是密码原语与密钥材料
 * （S 盒、轮函数、extendKey、SHA-256），模式封装与对外入口在 mainCircle.cpp。
 * 现在的对应关系：
 *     hashTo16Bytes / Aes::Aes / Aes::~Aes  <=> 原 hashTo16Bytes / extendKey（密钥建立与销毁）
 *     ctrXor                                <=> 原 subBytes/shiftRows/mixColumns（分组密码主体）
 */

// 密钥派生所需的轻量级 SHA256（仅用于把任意长度口令压成 128 位密钥）。
// 分组密码本体已改为 Windows CNG 的标准 AES，不再自研。

namespace {
    #define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
    #define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
    #define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
    #define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
    #define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
    #define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
    #define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

    const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    void sha256_compress(uint32_t state[8], const uint8_t *data) {
        uint32_t W[64];
        for (int i = 0; i < 16; i++) {
            W[i] = (data[i * 4] << 24) | (data[i * 4 + 1] << 16) | (data[i * 4 + 2] << 8) | data[i * 4 + 3];
        }
        for (int i = 16; i < 64; i++) {
            W[i] = SIG1(W[i - 2]) + W[i - 7] + SIG0(W[i - 15]) + W[i - 16];
        }
        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
        for (int i = 0; i < 64; i++) {
            uint32_t t1 = h + EP1(e) + CH(e, f, g) + K[i] + W[i];
            uint32_t t2 = EP0(a) + MAJ(a, b, c);
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }

    void sha256(const uint8_t *input, size_t len, uint8_t *output) {
        uint32_t state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                             0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
        size_t blocks = (len + 9 + 63) / 64;
        uint8_t *padded = new uint8_t[blocks * 64];
        memcpy(padded, input, len);
        padded[len] = 0x80;
        memset(padded + len + 1, 0, blocks * 64 - len - 1);
        uint64_t bitlen = len * 8;
        for (int i = 0; i < 8; i++) {
            padded[blocks * 64 - 1 - i] = (bitlen >> (i * 8)) & 0xff;
        }
        for (size_t i = 0; i < blocks; i++) {
            sha256_compress(state, padded + i * 64);
        }
        for (int i = 0; i < 8; i++) {
            output[i * 4] = (state[i] >> 24) & 0xff;
            output[i * 4 + 1] = (state[i] >> 16) & 0xff;
            output[i * 4 + 2] = (state[i] >> 8) & 0xff;
            output[i * 4 + 3] = state[i] & 0xff;
        }
        // padded 中暂存过明文口令，释放前必须清零（SecureZeroMemory 防被优化掉）
        SecureZeroMemory(padded, blocks * 64);
        delete[] padded;
    }
}

void Aes::hashTo16Bytes(const char *input, uint8_t *output) {
    uint8_t hash[32];
    sha256(reinterpret_cast<const uint8_t *>(input), strlen(input), hash);
    memcpy(output, hash, 16);
    // 完整摘要的前 16 字节即主密钥，栈上 32 字节副本一并清零
    SecureZeroMemory(hash, sizeof(hash));
}

Aes::Aes(const char *aesKey)
{
    memset(iv, 0, sizeof(iv));
    hashTo16Bytes(aesKey, aesKey16Bytes);

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0) < 0)
    {
        throw std::runtime_error("Failed to open AES algorithm provider");
    }

    // 用 ECB 批量加密计数器块来生成 CTR 密钥流。选这条路的实测依据：
    //   · CNG 原生 CTR：本机返回 STATUS_INVALID_PARAMETER(0xC000000D)，不可用
    //   · CNG CFB：既不是标准 CFB-128（IV 未直接参与首块），也不走硬件（54 MB/s）
    //   · ECB 批量：9.0 GB/s，是唯一可靠的硬件路径
    if (BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE,
                          (PUCHAR)BCRYPT_CHAIN_MODE_ECB,
                          sizeof(BCRYPT_CHAIN_MODE_ECB), 0) < 0)
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        hAlg = NULL;
        throw std::runtime_error("Failed to set AES chaining mode to ECB");
    }

    if (BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0,
                                   aesKey16Bytes, sizeof(aesKey16Bytes), 0) < 0)
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        hAlg = NULL;
        throw std::runtime_error("Failed to generate AES key");
    }
}

Aes::~Aes()
{
    // 主密钥与展开后的密钥句柄都能还原出完整加解密能力，销毁前必须一并处理
    SecureZeroMemory(aesKey16Bytes, sizeof(aesKey16Bytes));
    SecureZeroMemory(iv, sizeof(iv));
    if (hKey != NULL)
    {
        BCryptDestroyKey(hKey);
        hKey = NULL;
    }
    if (hAlg != NULL)
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        hAlg = NULL;
    }
}

void Aes::ctrXor(uint8_t *data, size_t len)
{
    if (len == 0)
    {
        return;
    }

    // 批量生成密钥流以摊薄 CNG 调用开销；CHUNK 必须是 16 的倍数
    const size_t CHUNK = 256 * 1024;
    ctrBuf.resize(CHUNK);
    ksBuf.resize(CHUNK);

    uint8_t ctr[16];
    memcpy(ctr, iv, 16);

    size_t offset = 0;
    while (offset < len)
    {
        const size_t remain = len - offset;
        const size_t fullBytes = (remain / 16) * 16;
        size_t thisLen = (fullBytes < CHUNK) ? fullBytes : CHUNK;

        if (thisLen == 0)
        {
            // 末尾不足 16 字节：生成整块密钥流，只消费前 remain 字节
            memcpy(ctrBuf.data(), ctr, 16);
            for (int i = 15; i >= 0; --i)
            {
                if (++ctr[i] != 0)
                    break; // 128 位大端计数器自增
            }
            ULONG done = 0;
            if (BCryptEncrypt(hKey, ctrBuf.data(), 16, NULL, NULL, 0,
                              ksBuf.data(), 16, &done, 0) < 0 || done != 16)
            {
                throw std::runtime_error("CTR keystream generation failed");
            }
            for (size_t i = 0; i < remain; ++i)
            {
                data[offset + i] ^= ksBuf[i];
            }
            offset += remain;
            continue;
        }

        for (size_t b = 0; b < thisLen; b += 16)
        {
            memcpy(ctrBuf.data() + b, ctr, 16);
            for (int i = 15; i >= 0; --i)
            {
                if (++ctr[i] != 0)
                    break;
            }
        }

        ULONG done = 0;
        if (BCryptEncrypt(hKey, ctrBuf.data(), static_cast<ULONG>(thisLen), NULL, NULL, 0,
                          ksBuf.data(), static_cast<ULONG>(thisLen), &done, 0) < 0 ||
            done != thisLen)
        {
            throw std::runtime_error("CTR keystream generation failed");
        }

        for (size_t i = 0; i < thisLen; ++i)
        {
            data[offset + i] ^= ksBuf[i];
        }
        offset += thisLen;
    }
}
