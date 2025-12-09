#include "../include/Aes.h"

using namespace std;

void Aes::aes(char *p, int plen)
{
    if (!p || plen <= 0)
        return;

    uint8_t feedback[16];
    memcpy(feedback, iv, 16);

    for (int offset = 0; offset < plen;)
    {
        int blockSize = min(16, plen - offset);

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
        int blockSize = min(16, clen - offset);
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

void Aes::processFileAES(const vector<char> inputBuffer, bool encrypt)
{
    // 打开文件
    vector<char> outPutBuffer;
    // 设置密钥
    setKey(aes_key);

    // 处理IV
    if (encrypt)
    {
        // 生成随机IV
        srand(static_cast<unsigned>(time(0)) ^ static_cast<unsigned>(reinterpret_cast<uintptr_t>(this)));
        for (int i = 0; i < 16; ++i)
        {
            iv[i] = static_cast<uint8_t>(rand() % 256);
        }
        outPutBuffer.push_back(reinterpret_cast<char>(iv));
    }
    else
    {
        memcpy(iv, inputBuffer.data(), 16);//读取IV
    }

    // 准备缓冲区
    vector<char> buffer(BUFFER_SIZE+1024);

    // 计算文件大小
    // inFile.seekg(0, ios::end);
    // streampos fileSize = inFile.tellg();
    // inFile.seekg(encrypt ? 0 : 16, ios::beg);
    // size_t remainingSize = static_cast<size_t>(fileSize) - (encrypt ? 0 : 16);

    cout << "处理中..." << endl;
    size_t totalProcessed = 0;

    // 处理文件内容
    while (true)
    {
        inFile.read(buffer.data(), buffer.size());
        streamsize bytesRead = inFile.gcount();
        if (bytesRead <= 0)
            break;

        // 调用核心加密/解密函数
        if (encrypt)
        {
            aes(buffer.data(), static_cast<int>(bytesRead));
        }
        else
        {
            deAes(buffer.data(), static_cast<int>(bytesRead));
        }

        // 写入处理后的数据
        outFile.write(buffer.data(), bytesRead);
        totalProcessed += bytesRead;

        // 显示进度
        // if (remainingSize > 0)
        // {
        //     cout << "\r进度: " << min(100, static_cast<int>(totalProcessed * 100 / remainingSize)) << "%" << flush;
        // }
    }

    // 安全清除
    memset(buffer.data(), 0, buffer.size());
    cout << "\n操作完成!" << endl;
}

int Aes::modeChoose(int mode)
{
    string keyInput;

    // cout << "AES加密/解密" << endl;
    // cout << "1. 加密" << endl;
    // cout << "2. 解密" << endl;
    // cout << "选择模式(1或2): ";
    cin >> mode;

    if (mode != 1 && mode != 2)
    {
        cerr << "无效模式选择!" << endl;
        return 0;
    }

    // cout << "密码: ";
    // cin.ignore();
    // getline(cin, keyInput);

    uint8_t aes_key[16] = {0};
    

    try
    {
        processFileAES(reinterpret_cast<const char *>(aes_key), mode == 1);
    }
    catch (const exception &e)
    {
        cerr << "错误: " << e.what() << endl;
        return 0;
    }

    // 清除敏感数据
    memset(aes_key, 0, sizeof(aes_key));

    system("pause");

    return 1;
}
