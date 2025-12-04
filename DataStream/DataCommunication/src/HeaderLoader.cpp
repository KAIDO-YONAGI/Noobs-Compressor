#include "../include/HeaderLoader.h"
#include "../include/Parser.hpp"
void BinaryIO_Loader::headerLoader(std::vector<std::string> &filePathToScan)
{
    try
    {

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
        NumsReader numsReader(inFile);
        DirectoryOffsetSize_uint offset = header->directoryOffset - HeaderSize;
        FileCount_uint countOfKidDirectory = 0;

        while (offset / BufferSize > 0 || offset % BufferSize > 0)
        {

            buffer.clear();
            loadBySepratedFlag(numsReader, offset, filePathToScan, countOfKidDirectory);
            // if (1)
            //     continue;//调试
            // 最后把目录数据块覆写回原位置（已经回填偏移量。如果有加密，则加密后再填，并且要在分割处写入iv头）
        }

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
void BinaryIO_Loader::loadBySepratedFlag(NumsReader &numsReader, DirectoryOffsetSize_uint &offset, std::vector<std::string> &filePathToScan, FileCount_uint &countOfKidDirectory)
{
    if (offset == 0)
        return;

    Parser paserForLoader(buffer,queue);
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

        DirectoryOffsetSize_uint bufferPtr = 0;

        while (readSize > bufferPtr) //(readSize<BufferSize)表示末尾块
        {
            while (countOfKidDirectory > 0 || bufferPtr == 0)
            {
                if (readSize <= bufferPtr)
                    return;
                std::pair<fs::path, char> result=paserForLoader.parser(tempOffset, bufferPtr, filePathToScan, countOfKidDirectory);
                //bufferPtr只在parser内自增，tempOffset需要调用模块自行管理
                // std::cout<<result.first<<result.second<<countOfD_F++;
                // int a = 0;
                // if (countOfD_F == 300)
                //     a++;
            }

            if (!queue.empty())
            {

                // countOfD_F++; // 临时全局变量
                std::cout
                    << queue.front().first.getFullPath()
                    // << " " << countOfD_F
                    << " " << queue.size()
                    << "\n";

                queue.pop();
                if (!queue.empty())
                    countOfKidDirectory = queue.front().second;
            }
            else
                break;
        }
        if (tempOffset == 0)
            offset -= readSize + sizeof(MagicNum); // tempOffset为零，说明到末尾，减去对应偏移量，包含魔数大小是为了结束循环
        // std::cout
        //     << "\n"
        //     << readSize
        //     << " " << offset
        //     << "\n"; // 调试
    }
    else
        throw std::runtime_error("loadBySepratedFlag()-Error:Failed to read separatedFlag");
}