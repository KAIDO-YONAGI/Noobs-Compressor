#include "../include/My_Aes.h"
DataBlock Aes::processDataAES(const DataBlock &inputBuffer, int  mode)
{
    DataBlock outputBuffer;

    // 处理IV
    if (mode==1)
    { // 加密
        // 生成随机IV
        if (RAND_priv_bytes(iv, sizeof(iv)) != 1)
        {
            throw std::runtime_error("Failed to generate random IV");
        }

        // 添加IV到输出缓冲区
        outputBuffer.insert(outputBuffer.end(), iv, iv + sizeof(iv));

        // 准备要加密的数据
        buffer = inputBuffer;
    }
    else if(mode ==2)
    { // 解密
        // 检查输入是否足够包含IV
        if (inputBuffer.size() < sizeof(iv))
        {
            throw std::runtime_error("Input too short to contain IV");
        }

        // 提取IV
        memcpy(iv, inputBuffer.data(), sizeof(iv));

        // 准备要解密的数据
        buffer.assign(inputBuffer.begin() + sizeof(iv), inputBuffer.end());
    }

    // 处理数据
    size_t bytesToProcess = buffer.size();
    if (mode==1)
    {
        aes(reinterpret_cast<char*>((buffer.data())), static_cast<int>(bytesToProcess)); // 加密
    }
    else if (mode==2)
    {
        deAes(reinterpret_cast<char*>((buffer.data())), static_cast<int>(bytesToProcess)); // 解密
    }

    // 添加处理后的数据到输出缓冲区
    outputBuffer.insert(outputBuffer.end(), buffer.begin(), buffer.end());

    return outputBuffer;
}
//mode 1: 加密 2: 解密
void Aes::doAes(int mode, const DataBlock &inputBuffer, DataBlock &outputBuffer)
{

    if (mode != 1 && mode != 2)
    {
        throw std::invalid_argument("Invalid mode. Use 1 for encryption and 2 for decryption.");
    }

    try
    {
        outputBuffer=processDataAES(inputBuffer, mode);
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
        int blockSize = std::min(16, plen - offset);

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
    memset(feedback, 0, sizeof(feedback));
}

void Aes::deAes(char *c, int clen)
{
    if (!c || clen <= 0)
        return;

    uint8_t feedback[16];
    memcpy(feedback, iv, 16);

    for (int offset = 0; offset < clen;)
    {
        int blockSize = std::min(16, clen - offset);
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
    memset(feedback, 0, sizeof(feedback));
}

