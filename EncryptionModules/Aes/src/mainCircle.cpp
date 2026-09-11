#include "../include/My_Aes.h"

#include <stdexcept>
#include <string>

/* AES 加解密主流程层：IV 生成/解析、统一对外入口。
 *
 * 与旧实现的分工保持一致：原 mainCircle.cpp 放的是 CFB 封装与 doAes
 * （IV 处理 + 模式循环 + 入口），底层密码原语与密钥材料在 AesFunctions.cpp。
 * 现在的对应关系：
 *     processDataAes  <=> 原 processDataAes（IV 前置/解析，加解密各自组装缓冲）
 *     doAes           <=> 原 doAes（非法 mode 校验 + 异常包装）
 * 原 aes()/deAes() 两个 CFB 循环已由 AesFunctions.cpp 的 ctrXor() 统一取代
 * （CTR 加解密同一条路径，无需再分两个函数）。
 */

Y_flib::Runtime::DataBlock Aes::processDataAes(const Y_flib::Runtime::DataBlock &inputBuffer, Y_flib::Runtime::AesMode mode)
{
    Y_flib::Runtime::DataBlock outputBuffer;

    if (mode == Y_flib::Runtime::AesMode::Encrypt)
    {
        // 每次加密生成新的随机 IV（系统首选 RNG，无需 CSP 句柄）
        if (BCryptGenRandom(NULL, iv, sizeof(iv), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
        {
            throw std::runtime_error("Failed to generate random IV");
        }

        // 一次分配：前置 IV，随后原地把明文加密进输出缓冲，省去中间整块拷贝
        outputBuffer.resize(sizeof(iv) + inputBuffer.size());
        memcpy(outputBuffer.data(), iv, sizeof(iv));
        if (!inputBuffer.empty())
        {
            memcpy(outputBuffer.data() + sizeof(iv), inputBuffer.data(), inputBuffer.size());
            ctrXor(outputBuffer.data() + sizeof(iv), inputBuffer.size());
        }
    }
    else if (mode == Y_flib::Runtime::AesMode::Decrypt)
    {
        // 检查输入是否足够包含 IV
        if (inputBuffer.size() < sizeof(iv))
        {
            throw std::runtime_error("Input too short to contain IV");
        }

        // 提取 IV
        memcpy(iv, inputBuffer.data(), sizeof(iv));

        // 密文体拷入输出后原地解密（CTR 与加密共用同一条密钥流路径）
        const size_t bodyLen = inputBuffer.size() - sizeof(iv);
        outputBuffer.resize(bodyLen);
        if (bodyLen > 0)
        {
            memcpy(outputBuffer.data(), inputBuffer.data() + sizeof(iv), bodyLen);
            ctrXor(outputBuffer.data(), bodyLen);
        }
    }

    return outputBuffer;
}

void Aes::doAes(Y_flib::Runtime::AesMode mode, const Y_flib::Runtime::DataBlock &inputBuffer, Y_flib::Runtime::DataBlock &outputBuffer)
{
    // 枚举值只能经 static_cast 越界，仍保留穷举校验兜底
    if (mode != Y_flib::Runtime::AesMode::Encrypt && mode != Y_flib::Runtime::AesMode::Decrypt)
    {
        throw std::invalid_argument("Invalid AesMode. Use AesMode::Encrypt or AesMode::Decrypt.");
    }

    try
    {
        outputBuffer = processDataAes(inputBuffer, mode);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("AES processing failed: ") + e.what());
    }
}
