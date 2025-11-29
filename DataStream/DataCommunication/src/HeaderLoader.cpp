#include "../include/HeaderLoader.h"

void BinaryIO_Loader::headerLoader()
{
    try
    {
        NumsReader numsReader(inFile);

        // 读取Header
        if (!inFile.read(reinterpret_cast<char *>(buffer.data()), HeaderSize))
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

        int countForTest = 0;

        while (offset / BufferSize > 0 || offset % BufferSize > 0)
        {
            // std::cout << offset << "\n";
            // std::cout << inFile.tellg() << "\n"; // 调试代码
            buffer.clear();
            loadBySepratedFlag(numsReader, offset);
        }
        // std::cout << countForTest << "\n";

        // 读末尾魔数并且检验
        SizeOfMagicNum_uint magicNum = numsReader.readBinaryNums<SizeOfMagicNum_uint>();
        if (magicNum != MagicNum)
            throw std::runtime_error("Invalid MagicNum");
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        // 清理资源或重新抛出异常
        throw;
    }
}
void BinaryIO_Loader::loadBySepratedFlag(NumsReader &numsReader, DirectoryOffsetSize_uint &offset)
{

    unsigned char flag = numsReader.readBinaryNums<unsigned char>();

    if (flag == '2')
    {

        // 读取块偏移量
        DirectoryOffsetSize_uint tempOffset = numsReader.readBinaryNums<DirectoryOffsetSize_uint>();
        // 读取iv头
        IvSize_uint ivNum = numsReader.readBinaryNums<IvSize_uint>();

        offset -= (SeparatedStandardSize + tempOffset); // 偏移量检测，同样用于检测退出（注意三目运算符的优先级）

        // 读取数据到vector后在内存中操作，对最后一个未达到写入分割标准大小的块引入特殊处理
        DirectoryOffsetSize_uint readSize = (tempOffset == 0 ? (offset - sizeof(SizeOfMagicNum_uint)) : tempOffset);

        // 按偏移量读取数据块
        buffer.resize(readSize); // clear后resize确保空间可写入，不改变capacity
        if (!inFile.read(reinterpret_cast<char *>(buffer.data()), readSize) && tempOffset != 0)
        {
            throw std::runtime_error("Failed to read buffer");
        }

        DirectoryOffsetSize_uint readed = inFile.gcount();
        if (tempOffset == 0)
            offset -= readed + sizeof(MagicNum); // tempOffset为零，说明到末尾，减去对应偏移量，包含魔数大小是为了结束循环

        std::cout << readed << " " << readSize << " " << offset << "\n";
        // std::cout << offset << "\n";// 调试代码

        DirectoryOffsetSize_uint bufferPtr = 0;

        fileParser(tempOffset, bufferPtr, readed);
    }
}
void BinaryIO_Loader::fileParser(DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &bufferPtr, DirectoryOffsetSize_uint readed)
{
    while (tempOffset > bufferPtr || readed > 0) //(可提取为函数)
    {

        // 解析flag
        unsigned char D_F_flag = numsParser<unsigned char>(bufferPtr);

        if (D_F_flag == '1') // 解析文件
        {
            // 解析文件名偏移量(可提取为函数)
            // 解析文件名，后续拼接为绝对路径之后交给数据读取类读取数据块
            FileNameSize_uint fileNameSize = 0;
            std::string fileName;
            fileNameSizeParser(fileNameSize, fileName, bufferPtr);

            // 解析文件原大小
            FileSize_uint originSize = numsParser<FileSize_uint>(bufferPtr);

            // 记录等会需要回填的位置
            FileSize_uint compressedSizeOffset = bufferPtr;
            bufferPtr += sizeof(FileSize_uint);

            readed -= FileStandardSize_Basic + fileNameSize;
            // countForTest++;
        }
        else if (D_F_flag == '0') // 解析目录
        {
            // 解析目录名偏移量(可提取为函数)
            // 解析目录名，后续拼接为绝对路径之后入队
            FileNameSize_uint directoryNameSize = 0;
            std::string directoryName;
            fileNameSizeParser(directoryNameSize, directoryName, bufferPtr);

            // 解析下级文件数量
            FileCount_uint count = numsParser<FileCount_uint>(bufferPtr);

            readed -= DirectoryrStandardSize_Basic + directoryNameSize;

            // 需要入队（BFS）

            // countForTest++;
        }

        else
            throw std::runtime_error("Failed to read flag");
    }
}
