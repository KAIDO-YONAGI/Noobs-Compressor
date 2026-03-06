#include "../include/BinaryIO_Loader.h"
namespace fs = std::filesystem;

void BinaryIO_Loader::headerLoaderIterator(Aes &aes)
{
    NumsReader numsReader(inFile);

    if (loaderRequestIsDone() || allLoopIsDone())
        return;

    inFile.seekg(header.directoryOffset - offset, std::ios::beg); // 定位到指定位置

    try
    {
        // 读取Header
        if (inFile.tellg() == std::streampos(0))
        {
            if (!inFile.read(reinterpret_cast<char *>(buffer.data()), HEADER_SIZE))
            {
                throw std::runtime_error("Failed to read header");
            }
            // 复制Header数据
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

        if (offset == sizeof(Y_flib::SizeOfMagicNum))
        {
            Y_flib::SizeOfMagicNum magicNum = numsReader.readBinaryNums<Y_flib::SizeOfMagicNum>();
            if (magicNum != MAGIC_NUM)
                throw std::runtime_error("Invalid MAGIC_NUM");

            allLoopDone(); // 完成所有循环后设置完成标志并return退出
            return;
        }
        while (offset > 0)
        {
            if (offset == 0)
                break;
            if (loaderRequestIsDone() || allLoopIsDone())
                return;

            buffer.clear();
            loadBySeparatedFlag(numsReader, countOfKidDirectory, aes);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        // 清理资源后重新抛出异常
        throw e.what();
    }
}

void BinaryIO_Loader::loadBySeparatedFlag(NumsReader &numsReader, Y_flib::FileCount &countOfKidDirectory, Aes &aes)
{
    if (offset == 0)
        return;

    const char flag = numsReader.readBinaryNums<char>();

    if (flag == SEPARATED_FLAG)
    {
        // 读取子块偏移量
        tempOffset = numsReader.readBinaryNums<Y_flib::DirectoryOffsetSize>();
        // 读取iv头
        Y_flib::IvSize ivNum = numsReader.readBinaryNums<Y_flib::IvSize>();

        offset -= SEPARATED_STANDARD_SIZE + tempOffset; // 偏移量减少，同时跳过固定头部长度

        // 读取加密数据到vector，等待解密处理：将读取到的数据块位置信息存入队列，供后续加密使用
        Y_flib::DirectoryOffsetSize readSize = (tempOffset == 0 ? (offset - sizeof(Y_flib::SizeOfMagicNum)) : tempOffset);

        std::array<Y_flib::DirectoryOffsetSize, 2> blockPos = {
            static_cast<Y_flib::DirectoryOffsetSize>(inFile.tellg()), // 转换为当前位置
            static_cast<Y_flib::DirectoryOffsetSize>(readSize)        // 转换为读取大小
        };
        pos.push_back(blockPos); // 记录数据块位置信息，供后续加密操作使用

        // 根据偏移量读取数据块
        buffer.resize(readSize); // clear和resize确保容器大小正确，避免残留数据
        if (!inFile.read(reinterpret_cast<char *>(buffer.data()), readSize) && tempOffset != 0)
        {
            throw std::runtime_error("Failed to read buffer");
        }

        if (ivNum != 0) // 当需要解压时，对buffer进行解密操作
        {
            Y_flib::DataBlock blockWithIv;
            Y_flib::DataBlock decryptedBlock;
            blockWithIv.resize(sizeof(Y_flib::IvSize));
            std::memcpy(blockWithIv.data(), &ivNum, sizeof(Y_flib::IvSize));
            blockWithIv.insert(blockWithIv.end(), buffer.begin(), buffer.end());

            aes.doAes(2, blockWithIv, decryptedBlock);
            buffer.clear();
            buffer.resize(decryptedBlock.size());
            buffer = decryptedBlock;
        }
        Y_flib::DirectoryOffsetSize bufferPtr = 0;

        while (readSize > bufferPtr)
        {
            while ((countOfKidDirectory > 0 || bufferPtr == 0) && readSize > bufferPtr)
            {
                parserForLoader->parser(bufferPtr, countOfKidDirectory);
            }

            if (!directory_FileQueue.empty() && countOfKidDirectory == 0)
            {
                // 目录队列处理逻辑
                const fs::path &directoryPath = directory_FileQueue.front().first.getFullPath();
                if (!directoryQueue_ready.empty())
                {
                    if (directoryQueue_ready.back() != directoryPath)
                    {
                        directoryQueue_ready.push(parentPath / directoryPath);
                    }
                }
                else if (FirstReady)
                {
                    directoryQueue_ready.push(parentPath / directoryPath);
                    FirstReady = false;
                }

                directory_FileQueue.pop();
                if (!directory_FileQueue.empty())
                {
                    countOfKidDirectory = directory_FileQueue.front().second; // 获取子目录数量
                    if (!directory_FileQueue.empty())
                        directoryQueue_ready.push(directory_FileQueue.front().first.getFullPath()); // pop前将当前目录加入，确保完整性
                }
            }
        }
        requestDone();       // 设置块完成标志
        if (tempOffset == 0) // tempOffset为0，说明到达末尾，减去相应偏移量
        {
            offset -= readSize;
            return;
        }
    }
    else
        throw std::runtime_error("loadBySeparatedFlag()-Error:Failed to read separatedFlag");
    return;
}
void BinaryIO_Loader::requestDone()
{

    blockIsDone = true;
}
void BinaryIO_Loader::allLoopDone()
{
    if (inFile.is_open())
    {
        inFile.close();
    }
    allDone = true;
}
void BinaryIO_Loader::restartLoader()
{
    if (!allLoopIsDone())
    {
        std::ifstream newInFile(loadPath, std::ios::binary);
        if (!newInFile)
            throw std::runtime_error("restartLoader()-Error:Failed to open inFile");

        size_t offsetToRestart = header.directoryOffset - offset;

        newInFile.seekg(offsetToRestart, std::ios::beg);
        this->inFile = std::move(newInFile);
        blockIsDone = false;
    }
    else
        return;
}
void BinaryIO_Loader::encryptHeaderBlock(Aes &aes)

{
    Y_flib::DataBlock inBlock;
    Y_flib::DataBlock encryptedBlock;
    Y_flib::DirectoryOffsetSize startPos = 0, blockSize = 0;

    for (auto blockPos : pos)
    {
        startPos = blockPos[0];
        blockSize = blockPos[1];

        inBlock.resize(blockSize);
        encryptedBlock.resize(blockSize + sizeof(Y_flib::IvSize));

        fstreamForRefill.seekp(startPos, std::ios::beg);
        fstreamForRefill.read(reinterpret_cast<char *>(inBlock.data()), blockSize);

        aes.doAes(1, inBlock, encryptedBlock);
        fstreamForRefill.seekp(startPos - sizeof(Y_flib::IvSize));
        fstreamForRefill.write(reinterpret_cast<const char *>(encryptedBlock.data()), blockSize + sizeof(Y_flib::IvSize));

        inBlock.clear();
        encryptedBlock.clear();
    }
}