#include "../include/HeaderLoader.h"
void BinaryIO_Loader::headerLoader()
{
    if (isDone)
        return;
    try
    {

        // 读取Header
        if (inFile.tellg() == std::streampos(0))
        {
            if (!inFile.read(reinterpret_cast<char *>(buffer.data()), HEADER_SIZE))
            {
                throw std::runtime_error("Failed to read header");
            }
            // 解释Header
            std::memcpy(&header, buffer.data(), sizeof(Header));
            // 验证魔数
            if (header.magicNum_1 != MAGIC_NUM ||
                header.magicNum_2 != MAGIC_NUM)
            {
                throw std::runtime_error("Invalid file format");
            }
            offset = header.directoryOffset - HEADER_SIZE;
        }

        NumsReader numsReader(inFile);

        while (offset / BUFFER_SIZE > 0 || offset % BUFFER_SIZE > 0)
        {
            buffer.clear();
            if (offset == 0)
                break;
            if (!fileQueue.empty())
                return;
            loadBySepratedFlag(numsReader, countOfKidDirectory);

            // if (1)
            //     continue;//调试
            // 最后把目录数据块覆写回原位置（已经回填偏移量。如果有加密，则加密后再填，并且要在分割处写入iv头）
        }
        if (offset == 0)
        {
            SizeOfMagicNum_uint magicNum = numsReader.readBinaryNums<SizeOfMagicNum_uint>();
            if (magicNum != MAGIC_NUM)
                throw std::runtime_error("Invalid MAGIC_NUM");

            std::cout << "All headers loaded successfully.\n";

            done();
            return;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        // 清理资源或重新抛出异常
        throw e.what();
    }
}
void BinaryIO_Loader::loadBySepratedFlag(NumsReader &numsReader, FileCount_uint &countOfKidDirectory)
{
    if (offset == 0)
        return;

    
    char flag = numsReader.readBinaryNums<char>();

    if (flag == '2')
    {

        // 读取块偏移量
        tempOffset = numsReader.readBinaryNums<DirectoryOffsetSize_uint>();
        // 读取iv头
        IvSize_uint ivNum = numsReader.readBinaryNums<IvSize_uint>();

        offset -= SEPARATED_STANDARD_SIZE + tempOffset; // 偏移量检测，同样用于检测退出

        // 读取数据到vector后在内存中操作，对最后一个未达到写入分割标准大小的块引入特殊处理
        DirectoryOffsetSize_uint readSize = (tempOffset == 0 ? (offset - sizeof(SizeOfMagicNum_uint)) : tempOffset);

        // 按偏移量读取数据块
        buffer.resize(readSize); // clear后resize确保空间可写入，不改变capacity
        if (!inFile.read(reinterpret_cast<char *>(buffer.data()), readSize) && tempOffset != 0)
        {
            throw std::runtime_error("Failed to read buffer");
        }

        DirectoryOffsetSize_uint bufferPtr = 0;

        if (readSize <= bufferPtr) //(readSize<BUFFER_SIZE)表示末尾块
            return;
        while (countOfKidDirectory > 0 || bufferPtr == 0)
        {
            if (readSize <= bufferPtr)
                break;
            parserForLoader->parser(bufferPtr, filePathToScan, countOfKidDirectory);

            // bufferPtr只在parser内自增，tempOffset需要调用模块自行管理
            //  std::cout<<result.first<<result.second<<countOfD_F++;
            //  int a = 0;
            //  if (countOfD_F == 300)
            //      a++;
        }

        if (!directoryQueue.empty())
        {

            // countOfD_F++; // 临时全局变量
            // std::cout
            //     << directoryQueue.front().first.getFullPath()
            //     // << " " << countOfD_F
            //     << " " << directoryQueue.size()
            //     // << " " << fileQueue.size()
            //     << "\n";

            // if (directoryQueue.front().first.getFullPath() == "D:\\1gal\\1h\\Tool\\locales\\SpcPeImageData.js")
            // {
            //     int a = 1;
            // }

            directoryQueue.pop();
            if (!directoryQueue.empty())
                countOfKidDirectory = directoryQueue.front().second;
        }

        if (tempOffset == 0)
        {
            offset -= readSize + sizeof(SizeOfMagicNum_uint); // tempOffset为零，说明到末尾，减去对应偏移量

            return;
        }

        // std::cout
        //     << "\n"
        //     << readSize
        //     << " " << offset
        //     << "\n"; // 调试
    }
    else
        throw std::runtime_error("loadBySepratedFlag()-Error:Failed to read separatedFlag");
    return;
}