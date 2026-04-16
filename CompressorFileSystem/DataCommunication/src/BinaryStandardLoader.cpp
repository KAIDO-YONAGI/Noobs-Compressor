#include "../include/BinaryStandardLoader.h"

void BinaryStandardLoader::headerLoaderIterator(Aes &aes)
{
    StandardsReader standardsReader(inFile);
    Locator locator;
    if (loaderRequestIsDone() || allLoopIsDone())
        return;
    locator.locateFromBegin(inFile, header.directoryOffset - offset); // 定位到指定位置

    try
    {
        // 读取Header
        if (!isReadHeader){
            loadHeaderStandard(inFile, header, buffer);
            isReadHeader = true;
        }

        if (offset == sizeof(Y_flib::SizeOfMagicNum))
        {
            Y_flib::SizeOfMagicNum magicNum = standardsReader.readBinaryStandards<Y_flib::SizeOfMagicNum>();
            if (magicNum != Y_flib::Constants::MAGIC_NUM)
                throw std::runtime_error("Invalid MAGIC_NUM");

            setAllLoopDone(); // 完成所有循环后设置完成标志并return退出
            return;
        }
        while (offset > 0)
        {
            if (offset == 0)
                break;
            if (loaderRequestIsDone() || allLoopIsDone())
                return;

            buffer.clear();
            loadEntryBlock(standardsReader, countOfChildDirectory, aes);
        }
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("BinaryStandardLoader encountered an error: ") + e.what());
    }
}

void BinaryStandardLoader::loadEntryBlock(StandardsReader &standardsReader, Y_flib::FileCount &countOfChildDirectory, Aes &aes)
{
    if (offset == 0)
        return;

    std::cout << "DEBUG loadEntryBlock: offset=" << offset << std::endl;

    Y_flib::FlagType flag;
    Y_flib::IvSize ivNum{};
    loadSeparatedStandard(const_cast<Y_flib::FlagType &>(flag), standardsReader, ivNum);

    // 读取加密数据到vector，等待解密处理：将读取到的数据块位置信息存入队列，供后续加密使用
    Y_flib::DirectoryOffsetSize readSize = (tempOffset == 0 ? (offset - sizeof(Y_flib::SizeOfMagicNum)) : tempOffset);

    std::cout << "DEBUG loadEntryBlock: tempOffset=" << tempOffset << ", readSize=" << readSize << std::endl;

    std::array<Y_flib::DirectoryOffsetSize, 2> blockPos = {
        static_cast<Y_flib::DirectoryOffsetSize>(inFile.tellg()), // 转换为当前位置
        static_cast<Y_flib::DirectoryOffsetSize>(readSize)        // 转换为读取大小(块大小)
    };
    blockPosition.push_back(blockPos); // 记录数据块位置信息，供后续加密操作使用

    // 根据偏移量读取数据块
    StandardsReader::readDataBlock(readSize, inFile, buffer);

    if (std::any_of(ivNum.begin(), ivNum.end(), [](auto byte) { return byte != 0; })) // 当需要解压时，对buffer进行解密操作
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
        while ((countOfChildDirectory > 0 || bufferPtr == 0) && readSize > bufferPtr)
        {
            parserForLoader->parser(bufferPtr, countOfChildDirectory);
        }

        if (!entryQueue.empty() && countOfChildDirectory == 0)
        {
            // 目录队列处理逻辑
            const std::filesystem::path &directoryPath = entryQueue.front().first.getFullPath();
            if (!directoryQueue_ready.empty())
            {
                if (directoryQueue_ready.back() != directoryPath)
                {
                    directoryQueue_ready.push(parentPath / directoryPath);
                }
            }
            else if (FirstReady) // 用firstReady让第一个元素入队，避免队列判空出问题
            {
                directoryQueue_ready.push(parentPath / directoryPath);
                FirstReady = false;
            }

            entryQueue.pop();
            if (!entryQueue.empty())
            {
                countOfChildDirectory = entryQueue.front().second; // 获取子目录数量
                if (!entryQueue.empty())
                    directoryQueue_ready.push(entryQueue.front().first.getFullPath()); // pop前将当前目录加入，确保完整性
            }
        }
    }
    setRequestDone();       // 设置块完成标志
    if (tempOffset == 0) // tempOffset为0，说明到达末尾，减去相应偏移量
    {
        offset -= readSize;
        return;
    }
}
void BinaryStandardLoader::loadHeaderStandard(std::ifstream &inFile, Y_flib::Header &header, Y_flib::DataBlock &buffer)
{
    // 读取Header
    if (inFile.tellg() == std::streampos(0))
    {
        StandardsReader::readDataBlock(Y_flib::Constants::HEADER_SIZE, inFile, buffer);
        // 复制Header数据
        std::memcpy(&header, buffer.data(), sizeof(Y_flib::Header));
        // 验证魔数
        if (header.magicNum_1 != Y_flib::Constants::MAGIC_NUM ||
            header.magicNum_2 != Y_flib::Constants::MAGIC_NUM)
        {
            throw std::runtime_error("Invalid file format");
        }
        if (header.directoryOffset == 0)
            throw std::runtime_error("Invalid directory offset in header");
        offset = header.directoryOffset - Y_flib::Constants::HEADER_SIZE;
        std::cout << "Header loaded successfully.\n";
    }
    else
    {
        throw std::runtime_error("Header already loaded or file pointer not at the beginning");
    }
}

void BinaryStandardLoader::loadSeparatedStandard(Y_flib::FlagType &flag, StandardsReader &standardsReader, Y_flib::IvSize &ivNum)
{
    flag = standardsReader.readBinaryStandards<Y_flib::FlagType>();

    // 读取子块偏移量
    tempOffset = standardsReader.readBinaryStandards<Y_flib::DirectoryOffsetSize>();
    // 读取iv头
    ivNum = standardsReader.readBinaryStandards<Y_flib::IvSize>();

    offset -= Y_flib::Constants::SEPARATED_STANDARD_SIZE + tempOffset; // 偏移量减少，同时步过固定头部长度
    if (flag != Y_flib::FlagType::Separated)
    {
        throw std::runtime_error("Invalid flag type for separated standard");
    }
}

void BinaryStandardLoader::encryptHeaderBlock(Aes &aes)

{
    Locator locator;
    Y_flib::DataBlock inBlock;
    Y_flib::DataBlock encryptedBlock;
    Y_flib::DirectoryOffsetSize startPos = 0, blockSize = 0;

    for (auto blockPos : blockPosition)
    {
        startPos = blockPos[0];
        blockSize = blockPos[1];

        inBlock.resize(blockSize);
        encryptedBlock.resize(blockSize + sizeof(Y_flib::IvSize));

        locator.locateFromBegin(fstreamForRefill, startPos); // 定位到数据块起始位置

        StandardsReader::readDataBlock(blockSize, fstreamForRefill, inBlock); // 读取数据块到buffer

        aes.doAes(1, inBlock, encryptedBlock);
        locator.locateFromBegin(fstreamForRefill, startPos - sizeof(Y_flib::IvSize)); // 定位回数据块起始位置，准备回写加密数据

        StandardsWriter::writeDataBlock(blockSize + sizeof(Y_flib::IvSize), fstreamForRefill, encryptedBlock); // 回写加密数据

        inBlock.clear();
        encryptedBlock.clear();
    }
}
void BinaryStandardLoader::setRequestDone()
{

    blockIsDone = true;
}
void BinaryStandardLoader::setAllLoopDone()
{
    if (inFile.is_open())
    {
        inFile.close();
    }
    allDone = true;
}
void BinaryStandardLoader::restartLoader()
{
    Locator locator;
    if (!allLoopIsDone())
    {
        std::ifstream newInFile(loadPath, std::ios::binary);
        if (!newInFile)
            throw std::runtime_error("restartLoader()-Error:Failed to open inFile");

        size_t offsetToRestart = header.directoryOffset - offset;

        locator.locateFromBegin(newInFile, offsetToRestart);
        this->inFile = std::move(newInFile);
        blockIsDone = false;
    }
}
