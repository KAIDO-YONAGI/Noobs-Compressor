#include "../include/HeaderLoader.h"

int main()
{
    Transfer transfer;
    BinaryIO_Loader loader;
    std::string inPath =
        "C:\\Users\\12248\\Desktop\\Secure Files Compressor\\DataStream\\DataCommunication\\bin\\挚爱的时光.bin";
    fs::path loadPath = transfer.transPath(inPath);
    std::ifstream inFile(loadPath, std::ios::binary);

    std::vector<char> buffer(BufferSize + 1024);
    try
    {
        // 检查文件是否打开
        if (!inFile.is_open())
        {
            throw std::runtime_error("File is not open");
        }

        // 读取Header
        buffer.resize(HeaderSize);
        if (!inFile.read(buffer.data(), HeaderSize))
        {
            throw std::runtime_error("Failed to read header");
        }

        // 解释Header
        const BinaryIO_Loader::Header *header =
            reinterpret_cast<const BinaryIO_Loader::Header *>(buffer.data());
        // 验证魔数
        if (header->magicNum_1 != MagicNum ||
            header->magicNum_2 != MagicNum)
        {
            throw std::runtime_error("Invalid file format");
        }

        DirectoryOffsetSize_uint offset = header->directoryOffset - HeaderSize;
        char flag;
        DirectoryOffsetSize_uint tempOffset = 0;

        while (offset > 0)
        {
            // 读取flag
            if (!inFile.read(&flag, FlagSize))
            {
                throw std::runtime_error("Failed to read flag");
            }

            if (flag == '2')
            {
                // 读取块偏移量
                if (!inFile.read(reinterpret_cast<char *>(&tempOffset),
                                 sizeof(DirectoryOffsetSize_uint)))
                {
                    throw std::runtime_error("Failed to read offset");
                }

                if (buffer.capacity() < tempOffset)
                    buffer.resize(tempOffset);

                // 读取数据
                if (!inFile.read(buffer.data(), tempOffset))
                {
                    throw std::runtime_error("Failed to read data");
                }
                while(tempOffset>0){

                }
                offset -= (SeparatedStandardSize - FlagSize)+tempOffset;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        // 清理资源或重新抛出异常
        throw;
    }

    system("pause");
    // size_t actualRead = inFile.gcount();
}
