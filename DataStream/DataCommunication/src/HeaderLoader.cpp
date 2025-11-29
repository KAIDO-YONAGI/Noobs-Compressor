#include "../include/HeaderLoader.h"

int main()
{
    Transfer transfer;

    std::string inPath =
        "C:\\Users\\12248\\Desktop\\Secure Files Compressor\\DataStream\\DataCommunication\\bin\\挚爱的时光.bin";
    fs::path loadPath = transfer.transPath(inPath);

    std::ifstream inFile(loadPath, std::ios::binary);
    std::vector<char> buffer(BufferSize + 1024);

    BinaryIO_Loader loader(buffer, inFile);
    loader.headerLoader();

    system("pause");
    // size_t actualRead = inFile.gcount();
}
void BinaryIO_Loader::headerLoader()
{
    try
    {
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
        NumsReader numsReader(inFile);
        while (offset > 0)
        {
            buffer.clear();
            char flag = numsReader.readBinaryNums<char>();

            if (flag == '2')
            {

                // 读取块偏移量
                DirectoryOffsetSize_uint tempOffset = numsReader.readBinaryNums<DirectoryOffsetSize_uint>();
                // 读取iv头
                IvSize_uint ivNum = numsReader.readBinaryNums<IvSize_uint>();
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
                    // 解析flag
                    char D_F_flag = numsParser(D_F_flag, bufferPtr);

                    if (D_F_flag == '1') // 解析文件
                    {
                        // 解析文件名偏移量(可提取为函数)
                        // 解析文件名，后续拼接为绝对路径之后交给数据读取类读取数据块
                        FileNameSize_uint fileNameSize = 0;
                        std::string fileName;
                        fileNameSizeParser(fileNameSize, fileName, bufferPtr);

                        // 解析文件原大小
                        FileSize_uint originSize = numsParser(originSize, bufferPtr);

                        // 记录等会需要回填的位置
                        FileSize_uint compressedSizeOffset = bufferPtr;
                    }
                    else if (D_F_flag == '0') // 解析目录
                    {
                        // 解析目录名偏移量(可提取为函数)
                        // 解析目录名，后续拼接为绝对路径之后入队
                        FileNameSize_uint directoryNameSize = 0;
                        std::string directoryName;
                        fileNameSizeParser(directoryNameSize, directoryName, bufferPtr);

                        // 解析下级文件数量
                        FileCount_uint count = numsParser(count, bufferPtr);

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
}
