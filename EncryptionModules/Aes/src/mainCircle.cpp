#include "Aes.h"

int Aes::modeChoose() {
    string inputFile, outputFile;
    string keyInput;
    int mode;

    cout << "AES Encryption/Decryption" << endl;
    cout << "1. Encryption" << endl;
    cout << "2. Decryption" << endl;
    cout << "Choose mode(1 or 2): ";
    cin >> mode;

    if (mode != 1 && mode != 2) {
        cerr << "FQ" << endl;
        return 0;
    }

    cout << "Pwd: ";
    cin.ignore();
    getline(cin, keyInput);
    
    uint8_t aes_key[16];
    hash_to_16bytes(keyInput.c_str(), aes_key);
    
    cout << "Input" << (mode == 1 ? "Source" : "Encrypted Files") << ": ";
    cin >> inputFile;

    cout << "Output" << (mode == 1 ? "Encrypt Files" : "Decrypt Files") << ": ";
    cin >> outputFile;

    try {
        processFileAES(inputFile, outputFile, (char*)aes_key, mode == 1);
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 0;
    }
    system("pause");
    return 1;
}

void Aes::processFileAES(const string& inputFile, const string& outputFile, 
                         const char* aes_key, bool encrypt) {
    ifstream inFile(inputFile, ios::binary);
    ofstream outFile(outputFile, ios::binary);
    
    if (!inFile || !outFile) {
        cerr << "Can not open the file!" << endl;
        return;
    }

    // 生成随机IV
    srand(time(0));
    for(int i = 0; i < 16; i++) {
        iv[i] = rand() % 256;
    }
    if(encrypt) {
        // 加密时写入IV到输出文件开头
        outFile.write((char*)iv, 16);
    } else {
        // 解密时从输入文件读取IV
        inFile.read((char*)iv, 16);
    }

    size_t fileSize = inFile.tellg();
    inFile.seekg(0, ios::beg);
    if(!encrypt) fileSize -= 16; // 减去IV大小

    size_t totalBlocks = (fileSize + BUFFER_SIZE - 1) / BUFFER_SIZE;
    size_t remainingBytes = fileSize % BUFFER_SIZE;
    if (remainingBytes == 0) remainingBytes = BUFFER_SIZE;

    vector<char> buffer(BUFFER_SIZE);
    size_t processedBytes = 0;
    size_t blockCount = 0;

    while (blockCount < totalBlocks) {
        size_t currentBlockSize = (blockCount == totalBlocks - 1) ? remainingBytes : BUFFER_SIZE;
        
        // 读取数据块
        inFile.read(buffer.data(), currentBlockSize);
        size_t bytesRead = inFile.gcount();
        
        if (bytesRead == 0) break;

        // 执行加密/解密
        if (encrypt) {
            aes(buffer.data(), bytesRead, aes_key);
        } else {
            deAes(buffer.data(), bytesRead, aes_key);
        }

        // 写入处理后的数据
        outFile.write(buffer.data(), bytesRead);

        processedBytes += bytesRead;
        blockCount++;
        
        // 显示进度
        int progress = static_cast<int>((processedBytes * 100) / fileSize);
        cout << "\rProgress: " << progress << "%" << flush;
    }

    cout << endl << "Done!" << endl;
    inFile.close();
    outFile.close();
}

void Aes::aes(char *p, int plen, const char *key) {
    extendKey(key);
    uint8_t feedback[16];
    memcpy(feedback, iv, 16); // 初始反馈寄存器设置为IV
    
    for(int k = 0; k < plen; k += 16) {
        int blockSize = (plen - k) < 16 ? (plen - k) : 16;
        int pArray[4][4];
        
        // 加密反馈寄存器
        convertToIntArray((char*)feedback, pArray);
        addRoundKey(pArray, 0);
        for(int i = 1; i < 10; i++) {
            subBytes(pArray);
            shiftRows(pArray);
            mixColumns(pArray);
            addRoundKey(pArray, i);
        }
        subBytes(pArray);
        shiftRows(pArray);
        addRoundKey(pArray, 10);
        convertArrayToStr(pArray, (char*)feedback);
        
        // 与明文异或
        for(int i = 0; i < blockSize; i++) {
            p[k+i] ^= feedback[i];
        }
        
        // 更新反馈寄存器
        if(blockSize == 16) {
            memcpy(feedback, p + k, 16);
        } else {
            // 部分块处理
            memcpy(feedback, p + k, blockSize);
        }
    }
}

void Aes::deAes(char *c, int clen, const char *key) {
    extendKey(key);
    uint8_t feedback[16];
    memcpy(feedback, iv, 16); // 初始反馈寄存器设置为IV
    
    for(int k = 0; k < clen; k += 16) {
        int blockSize = (clen - k) < 16 ? (clen - k) : 16;
        int cArray[4][4];
        uint8_t temp[16];
        
        // 保存当前密文块
        if(blockSize == 16) {
            memcpy(temp, c + k, 16);
        }
        
        // 加密反馈寄存器
        convertToIntArray((char*)feedback, cArray);
        addRoundKey(cArray, 0);
        for(int i = 1; i < 10; i++) {
            subBytes(cArray);
            shiftRows(cArray);
            mixColumns(cArray);
            addRoundKey(cArray, i);
        }
        subBytes(cArray);
        shiftRows(cArray);
        addRoundKey(cArray, 10);
        convertArrayToStr(cArray, (char*)feedback);
        
        // 与密文异或
        for(int i = 0; i < blockSize; i++) {
            c[k+i] ^= feedback[i];
        }
        
        // 更新反馈寄存器
        if(blockSize == 16) {
            memcpy(feedback, temp, 16);
        } else {
            // 部分块处理
            memcpy(feedback, temp, blockSize);
        }
    }
}

