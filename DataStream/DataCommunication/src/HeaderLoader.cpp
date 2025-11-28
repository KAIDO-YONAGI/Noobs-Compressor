#include "../include/HeaderLoader.h"

int main()
{
    Transfer transfer;
    BinaryIO_Loader loader;
    std::string inPath =
        "C:\\Users\\12248\\Desktop\\Secure Files Compressor\\DataStream\\DataCommunication\\bin\\挚爱的时光.bin";
    fs::path loadPath = transfer.transPath(inPath);
    std::ifstream inFile(loadPath, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "无法打开文件" << std::endl;
        return 1;
    }
    std::vector<char> buffer(BufferSize+1024);
    inFile.read(buffer.data(),HeaderSize);
    const BinaryIO_Loader::Header* header = reinterpret_cast<const BinaryIO_Loader::Header*>(buffer.data());
    std::cout<<header->MagicNum;

    system("pause");
    // size_t actualRead = inFile.gcount();


}
