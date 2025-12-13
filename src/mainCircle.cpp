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
<<<<<<<< HEAD:src/mainCircle.cpp
ifstream inFile(inputFile, ios::binary);
========
    ifstream inFile(inputFile, ios::binary);
>>>>>>>> koharu:EncryptionModules/Aes/src/mainCircle.cpp
    ofstream outFile(outputFile, ios::binary);
    
    if (!inFile || !outFile) {
        cerr << "Can not open the file!" << endl;
        return;
    }

<<<<<<<< HEAD:src/mainCircle.cpp
    // 获取文件大小
========
    // ��ȡ�ļ���С
>>>>>>>> koharu:EncryptionModules/Aes/src/mainCircle.cpp
    inFile.seekg(0, ios::end);
    size_t fileSize = inFile.tellg();
    inFile.seekg(0, ios::beg);

<<<<<<<< HEAD:src/mainCircle.cpp
    // 计算需要的处理和块大小
    size_t totalBlocks = (fileSize + BUFFER_SIZE - 1) / BUFFER_SIZE;
========
    // ������Ҫ�Ĵ����Ϳ��С
    size_t totalBlocks = (fileSize + BUFFER_SIZE - 1) / BUFFER_SIZE;//����ȡ����ȡ������ת��
>>>>>>>> koharu:EncryptionModules/Aes/src/mainCircle.cpp
    size_t remainingBytes = fileSize % BUFFER_SIZE;
    if (remainingBytes == 0) remainingBytes = BUFFER_SIZE;

    vector<char> buffer(BUFFER_SIZE);
    size_t processedBytes = 0;
    size_t blockCount = 0;

    while (blockCount < totalBlocks) {
        size_t currentBlockSize = (blockCount == totalBlocks - 1) ? remainingBytes : BUFFER_SIZE;
        size_t paddedBlockSize = currentBlockSize; // 定义paddedBlockSize并初始化为当前块大小
        
        // 读取数据块
        inFile.read(buffer.data(), currentBlockSize);
        size_t bytesRead = inFile.gcount();
        
        if (bytesRead == 0) break;

        // 加密时添加填充
        if (encrypt) {
            if (bytesRead % 16 != 0) {
                size_t paddingSize = 16 - (bytesRead % 16);
                paddedBlockSize = bytesRead + paddingSize;
                buffer.resize(paddedBlockSize);
                memset(buffer.data() + bytesRead, paddingSize, paddingSize);  // PKCS#7填充
            }
        }

        // 执行加密/解密
        if (encrypt) {
            aes(buffer.data(), paddedBlockSize, aes_key);
        } else {
            deAes(buffer.data(), paddedBlockSize, aes_key);
            // 解密后去除填充
            if (blockCount == totalBlocks - 1) {
                uint8_t paddingSize = static_cast<uint8_t>(buffer[paddedBlockSize - 1]);
                if (paddingSize <= 16) {
                    paddedBlockSize -= paddingSize;
                }
            }
        }

        // 写入处理后的数据
        outFile.write(buffer.data(), paddedBlockSize);

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
int pArray[4][4];
extendKey(key);for(int k = 0; k < plen; k += 16) {
    convertToIntArray(p + k, pArray);
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
    convertArrayToStr(pArray, p + k);
<<<<<<<< HEAD:src/mainCircle.cpp
========
}
}

void Aes::deAes(char *c, int clen, const char *key) {
    int cArray[4][4];
	int k;
	extendKey(key);

	for(k = 0; k < clen; k += 16) {
		int i;
		int wArray[4][4];

		convertToIntArray(c + k, cArray);
		addRoundKey(cArray, 10);

		for(i = 9; i >= 1; i--) {
			deSubBytes(cArray);

			deShiftRows(cArray);

			deMixColumns(cArray);
			getArrayFrom4W(i, wArray);
			deMixColumns(wArray);

			addRoundTowArray(cArray, wArray);
		}

		deSubBytes(cArray);

		deShiftRows(cArray);

		addRoundKey(cArray, 0);

		convertArrayToStr(cArray, c + k);

	}
>>>>>>>> koharu:EncryptionModules/Aes/src/mainCircle.cpp
}
}

void Aes::deAes(char *c, int clen, const char *key) {
    int cArray[4][4];
	int k;
	extendKey(key);

	for(k = 0; k < clen; k += 16) {
		int i;
		int wArray[4][4];

		convertToIntArray(c + k, cArray);
		addRoundKey(cArray, 10);

		for(i = 9; i >= 1; i--) {
			deSubBytes(cArray);

			deShiftRows(cArray);

			deMixColumns(cArray);
			getArrayFrom4W(i, wArray);
			deMixColumns(wArray);

			addRoundTowArray(cArray, wArray);
		}

		deSubBytes(cArray);

		deShiftRows(cArray);

		addRoundKey(cArray, 0);

		convertArrayToStr(cArray, c + k);

	}
}