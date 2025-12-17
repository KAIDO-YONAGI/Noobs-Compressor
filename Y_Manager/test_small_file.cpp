// 测试小文件压缩解压
#include "../EncryptionModules/Aes/include/My_Aes.h"
#include "../CompressionModules/heffman/include/Heffman.h"
#include "../DataBlocks/DataBlocksManage.h"
#include <fstream>
#include <iostream>

int main() {
    // 读取原始文件
    std::ifstream inFile("D:\\1gal\\1h\\Tool\\node_modules\\adm-zip\\methods\\deflater.js", std::ios::binary);
    if (!inFile) {
        std::cerr << "Cannot open input file\n";
        return 1;
    }

    DataBlock originalData((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    std::cout << "Original file size: " << originalData.size() << " bytes\n";

    // 压缩
    Heffman compressor(1);
    compressor.statistic_freq(0, originalData);
    compressor.merge_ttabs();
    compressor.gen_hefftree();
    compressor.save_code_inTab();

    DataBlock compressedData;
    compressor.encode(originalData, compressedData);
    std::cout << "Compressed size: " << compressedData.size() << " bytes\n";

    // 加密
    Aes aes("TESTKEY");
    DataBlock encryptedData;
    aes.doAes(1, compressedData, encryptedData);
    std::cout << "Encrypted size: " << encryptedData.size() << " bytes\n";

    // 解密
    DataBlock decryptedData;
    aes.doAes(2, encryptedData, decryptedData);
    std::cout << "Decrypted size: " << decryptedData.size() << " bytes\n";

    // 解压
    Heffman decompressor(1);
    DataBlock huffTree;
    compressor.tree_to_plat_uchar(huffTree);
    decompressor.spawn_tree(huffTree);

    DataBlock decompressedData;
    decompressor.decode(decryptedData, decompressedData, BitHandler(), originalData.size());

    std::cout << "Decompressed size: " << decompressedData.size() << " bytes\n";

    // 验证
    if (originalData.size() == decompressedData.size()) {
        bool identical = true;
        for (size_t i = 0; i < originalData.size(); ++i) {
            if (originalData[i] != decompressedData[i]) {
                identical = false;
                std::cout << "Difference at position " << i << "\n";
                break;
            }
        }
        if (identical) {
            std::cout << "SUCCESS: Files are identical!\n";
        } else {
            std::cout << "FAIL: Files differ\n";
        }
    } else {
        std::cout << "FAIL: Size mismatch\n";
    }

    return 0;
}
