#include "../include/Aes.h"

//加密8KB数据块示例
// int main() {
//     std::vector<char> inputBuffer(BUFFER_SIZE); // 初始化8KB大小的缓冲区
//     std::ifstream file("C:\\Users\\12248\\Desktop\\Secure Files Compressor\\DataStream\\DataCommunication\\bin\\挚爱的时光.bin", 
//                       std::ios::binary);
    
//     if (!file) {
//         std::cerr << "无法打开文件！" << std::endl;
//         return 1;
//     }

//     // 读取8KB数据块
//     file.read(inputBuffer.data(), BUFFER_SIZE);
    
//     // 检查实际读取的字节数
//     size_t bytesRead = file.gcount();
//     if (bytesRead < BUFFER_SIZE) {
//         inputBuffer.resize(bytesRead); // 调整大小为实际读取的数据量
//     }
//     std::ofstream outputFile(
//         "加密结果.bin",
//         std::ios::binary
//     );

//     Aes aes("your_aes_key_here");
//     std::vector<char> outputBuffer = aes.runAES(1, inputBuffer); // 示例调用

//     outputFile.write(outputBuffer.data(), outputBuffer.size());
//     outputFile.close();

//     return 0;
// }


//解密8KB数据块示例
//int main() {
//    std::vector<char> inputBuffer(BUFFER_SIZE + 16); // 初始化8KB大小的缓冲区，额外16字节用于IV
//    std::ifstream file("加密结果.bin", 
//                      std::ios::binary);
//    
//    if (!file) {
//        std::cerr << "无法打开文件！" << std::endl;
//        return 1;
//    }
//
//    // 读取8KB数据块加上IV
//    file.read(inputBuffer.data(), BUFFER_SIZE + 16);
//    
//    // 检查实际读取的字节数
//    size_t bytesRead = file.gcount();
//    if (bytesRead < BUFFER_SIZE + 16) {
//        inputBuffer.resize(bytesRead); // 调整大小为实际读取的数据量
//    }
//    std::ofstream outputFile(
//        "解密结果.bin",
//        std::ios::binary
//    );
//
//    Aes aes("your_aes_key_here");
//    std::vector<char> outputBuffer = aes.runAES(2, inputBuffer); // 示例调用
//
//    outputFile.write(outputBuffer.data(), outputBuffer.size());
//    outputFile.close();
//
//    return 0;
//}
