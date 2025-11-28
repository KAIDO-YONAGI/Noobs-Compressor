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

        while (offset > 0)
        {
            buffer.clear();
            char flag = '\0';
            DirectoryOffsetSize_uint tempOffset = 0;
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

                // 补全不足的长度
                if (buffer.capacity() < tempOffset)
                    buffer.resize(tempOffset);

                // 读取数据到vector后在内存中操作，对最后一个未达到写入分割标准的块引入特殊处理
                if (!inFile.read(buffer.data(), tempOffset == 0 ? BufferSize : tempOffset))
                {
                    throw std::runtime_error("Failed to read data");
                }

                if (buffer.size() == 0)
                    break; // 已经读完，退出循环，避免以下操作vector越界

                DirectoryOffsetSize_uint bufferPtr = 0;
                while (tempOffset < bufferPtr) //(可提取为函数)
                {
                    char D_F_flag = '\0';
                    // 解析flag
                    memcpy(&D_F_flag, buffer.data() + bufferPtr, sizeof(char));
                    bufferPtr += FlagSize;
                    if (D_F_flag == '1') // 解析文件
                    {
                        // 解析文件名偏移量(可提取为函数)
                        FileNameSize_uint fileNameSize = 0;
                        memcpy(&fileNameSize, buffer.data() + bufferPtr, sizeof(FileNameSize_uint));
                        bufferPtr += sizeof(FileNameSize_uint);

                        // 解析文件名，后续拼接为绝对路径之后交给数据读取类读取数据块
                        std::string fileName;
                        memcpy(&fileName, buffer.data() + bufferPtr, fileNameSize);
                        bufferPtr += fileName.size();

                        // 解析文件原大小
                        FileSize_uint originSize = 0;
                        memcpy(&originSize, buffer.data() + bufferPtr, sizeof(FileSize_uint));
                        bufferPtr += sizeof(FileSize_uint);
                    }
                    else if (D_F_flag == '0') // 解析目录
                    {
                        // 解析目录名偏移量(可提取为函数)
                        FileNameSize_uint directoryNameSize = 0;
                        memcpy(&directoryNameSize, buffer.data() + bufferPtr, sizeof(FileNameSize_uint));
                        bufferPtr += sizeof(FileNameSize_uint);

                        // 解析目录名，后续拼接为绝对路径之后入队
                        std::string directoryName;
                        memcpy(&directoryName, buffer.data() + bufferPtr, directoryNameSize);
                        bufferPtr += directoryName.size();

                        // 解析第一级文件数量
                        FileCount_uint count = 0;
                        memcpy(&count, buffer.data() + bufferPtr, sizeof(FileCount_uint));
                        bufferPtr += sizeof(FileCount_uint);

                        // 需要入队（BFS）
                    }

                    else
                        throw std::runtime_error("Failed to read flag");
                }
                offset -= (SeparatedStandardSize - FlagSize) + tempOffset; // 偏移量检测，同样用于检测退出
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
