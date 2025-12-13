#include "../include/BinaryIO_Loader.h"
void BinaryIO_Loader::headerLoaderIterator(Aes &aes)
{
    if (loaderRequestIsDone() || allLoopIsDone())
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
            if (header.directoryOffset == 0)
                throw std::runtime_error("Invalid directory offset in header");
            offset = header.directoryOffset - HEADER_SIZE;
        }

        NumsReader numsReader(inFile);
        if (offset == sizeof(SizeOfMagicNum_uint))
        {
            SizeOfMagicNum_uint magicNum = numsReader.readBinaryNums<SizeOfMagicNum_uint>();
            if (magicNum != MAGIC_NUM)
                throw std::runtime_error("Invalid MAGIC_NUM");
            allLoopDone();
            return;
        }
        while (offset > 0)
        {
            buffer.clear();
            if (offset == 0)
                break;
            if (loaderRequestIsDone() || allLoopIsDone())
                return;
            loadBySepratedFlag(numsReader, countOfKidDirectory, aes);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        // 清理资源或重新抛出异常
        throw e.what();
    }
}
void BinaryIO_Loader::loadBySepratedFlag(NumsReader &numsReader, FileCount_uint &countOfKidDirectory, Aes &aes)
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

        std::array<DirectoryOffsetSize_uint, 2> blockPos = {
            static_cast<DirectoryOffsetSize_uint>(inFile.tellg()), // 转换流位置
            static_cast<DirectoryOffsetSize_uint>(readSize)        // 转换读取大小
        };
        pos.push_back(blockPos); // 保存此时数据块的位置，便于下游对数据块直接操作

        // 按偏移量读取数据块
        buffer.resize(readSize); // clear后resize确保空间可写入，不改变capacity
        if (!inFile.read(reinterpret_cast<char *>(buffer.data()), readSize) && tempOffset != 0)
        {
            throw std::runtime_error("Failed to read buffer");
        }

        if (ivNum != 0) // 仅在解压时会触发，将buffer解密后解析
        {
            DataBlock blockWithIv;
            DataBlock dencryptedBlock;

            blockWithIv.push_back(ivNum);
            blockWithIv.insert(blockWithIv.end(), buffer.begin(), buffer.end());

            aes.doAes(2, blockWithIv, dencryptedBlock);
            buffer = dencryptedBlock;
        }
        DirectoryOffsetSize_uint bufferPtr = 0;

        while (readSize > bufferPtr)
        {

            while ((countOfKidDirectory > 0 || bufferPtr == 0) && readSize > bufferPtr)
            {
                parserForLoader->parser(bufferPtr, countOfKidDirectory);
            }

            if (!directoryQueue.empty() && countOfKidDirectory == 0)
            {
                //目录进入就绪队列的逻辑
                const fs::path &directoryPath = directoryQueue.front().first.getFullPath();
                if (!directoryQueue_ready.empty())
                {
                    if (directoryQueue_ready.back() != directoryPath)
                    {
                        directoryQueue_ready.push(directoryPath);
                    }
                }
                else
                    directoryQueue_ready.push(directoryPath);
                
                directoryQueue.pop();
                if (!directoryQueue.empty())
                {
                    countOfKidDirectory = directoryQueue.front().second;//更新子目录数量
                    directoryQueue_ready.push(directoryQueue.front().first.getFullPath());//pop后直接将新目录入队，防止处理到一半，没有进入外层if
                }
            }
        }
        requesetDone();      // 单次请求完成
        if (tempOffset == 0) // tempOffset为零，说明到末尾，减去对应偏移量
        {
            offset -= readSize + sizeof(SizeOfMagicNum_uint);
            return;
        }
    }
    else
        throw std::runtime_error("loadBySepratedFlag()-Error:Failed to read separatedFlag");
    return;
}
