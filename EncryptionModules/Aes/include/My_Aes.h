#pragma once

#include <windows.h>
#include <bcrypt.h>

#include <cstdint>
#include <cstring>
#include <vector>

#include "../../../CompressorFileSystem/Commons/include/FileLibrary.h"

/* AES-128-CTR 加密（标准 AES，Windows CNG/BCrypt，自动走 AES-NI 硬件指令）

   归档布局与旧实现一致：[16 字节随机 IV][密文体]，密文体长度 == 明文长度。

   为什么换掉旧核心：旧实现是"自研 AES-128 CFB"，其 MixColumns 混合轴有误
   （对同一行跨列混合；标准 AES 是对同一列混合），实测单 bit 翻转的密文差异
   128/128 例全部局限在同一行，状态矩阵 4 行互不影响、不满足分组密码的雪崩判据；
   同时密文与标准 AES 不互通，也没有任何硬件加速路径。已确认无历史归档需求，
   故直接改为标准实现，不再保留旧核心。

   实现要点：
     · 密钥派生沿用 SHA256(口令) 前 16 字节
     · IV 仍为每次加密随机生成的 16 字节前置（BCryptGenRandom）
     · CTR 是流模式，无需填充 => 密文体长度与明文完全相同，字节开销不变
     · 本机 CNG 的原生 CTR 返回 STATUS_INVALID_PARAMETER，CFB 既不标准又不走硬件，
       故用 ECB 批量加密计数器块生成密钥流（实测 9.0 GB/s）

   Aes 实例按模块私有使用，非线程安全（持有 CNG 密钥句柄与复用缓冲）。 */
class Aes
{
public:
    /* 初始化：派生密钥、打开算法提供者、生成密钥句柄。失败抛 runtime_error */
    explicit Aes(const char *aesKey);

    /* 持有 CNG 密钥/提供者句柄后不可复制（否则双重释放） */
    Aes(const Aes &) = delete;
    Aes &operator=(const Aes &) = delete;

    /* 析构：清零密钥副本并释放 CNG 句柄 */
    ~Aes();

    /* 统一加密/解密接口，按 AesMode 选择加密或解密。自动分块处理 */
    void doAes(Y_flib::AesMode mode, const Y_flib::DataBlock &inputBuffer, Y_flib::DataBlock &outputBuffer);

private:
    /* CTR 密钥流生成并与 data 原地异或（len 可为任意长度） */
    void ctrXor(uint8_t *data, size_t len);

    /* 将任意长度密钥哈希为 128 位，使用 SHA-256 并截取前 16 字节 */
    void hashTo16Bytes(const char *input, uint8_t *output);

    /* 分块处理，按 AesMode 走加密/解密；IV 前置/解析在此完成 */
    Y_flib::DataBlock processDataAes(const Y_flib::DataBlock &inputBuffer, Y_flib::AesMode mode);

    BCRYPT_ALG_HANDLE hAlg = NULL; // AES 算法提供者（ECB 链模式，用于批量生成密钥流）
    BCRYPT_KEY_HANDLE hKey = NULL; // 展开后的密钥句柄
    uint8_t iv[16];                // 当前块的 IV / 计数器初值
    uint8_t aesKey16Bytes[16];     // 128 位主密钥（哈希后）
    Y_flib::DataBlock ctrBuf;      // 计数器块缓冲（复用，避免逐块分配）
    Y_flib::DataBlock ksBuf;       // 密钥流缓冲（复用）
};
