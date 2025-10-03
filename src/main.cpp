#include "Aes.h"


using namespace std;

const size_t BUFFER_SIZE = 4 * 1024 * 1024; // 4MB缓冲区

// 生成16字节的AES密钥
void hash_to_16bytes(const char* input, uint8_t output[16]) {
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256((const uint8_t*)input, strlen(input), hash);
    memcpy(output, hash, 16);
}

void processFileAES(const string& inputFile, const string& outputFile, 
                   const char* aes_key, bool encrypt) {
    ifstream inFile(inputFile, ios::binary);
    ofstream outFile(outputFile, ios::binary);
    
    if (!inFile || !outFile) {
        cerr << "无法打开文件!" << endl;
        return;
    }

    
    // 获取文件大小
    inFile.seekg(0, ios::end);
    size_t fileSize = inFile.tellg();
    inFile.seekg(0, ios::beg);

    // 计算需要的处理和块大小
    size_t totalBlocks = (fileSize + BUFFER_SIZE - 1) / BUFFER_SIZE;
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
        cout << "\r处理进度: " << progress << "%" << flush;
    }

    cout << endl << "文件处理完成!" << endl;
    inFile.close();
    outFile.close();
}

int main() {
    string inputFile, outputFile;
    string keyInput;
    int mode;

    cout << "AES 文件加密/解密" << endl;
    cout << "1. 加密" << endl;
    cout << "2. 解密" << endl;
    cout << "选择模式 (1/2): ";
    cin >> mode;

    if (mode != 1 && mode != 2) {
        cerr << "无效模式选择!" << endl;
        return 1;
    }

    cout << "输入密钥: ";
    cin.ignore();
    getline(cin, keyInput);
    
    uint8_t aes_key[16];
    hash_to_16bytes(keyInput.c_str(), aes_key);
    
    cout << "输入" << (mode == 1 ? "源文件" : "加密文件") << ": ";
    cin >> inputFile;

    cout << "输出" << (mode == 1 ? "加密文件" : "解密文件") << ": ";
    cin >> outputFile;

    try {
        processFileAES(inputFile, outputFile, (char*)aes_key, mode == 1);
    } catch (const exception& e) {
        cerr << "处理错误: " << e.what() << endl;
        return 1;
    }
    system("pause");
    return 0;
}
