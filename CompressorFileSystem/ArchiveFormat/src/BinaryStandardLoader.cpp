#include "../include/BinaryStandardLoader.h"
#include "../include/StrategyFactory.h"

namespace Y_flib
{
    void BinaryStandardLoader::headerLoaderIterator(Y_flib::IEncryption &encryption)
    {
        StandardsReader standardsReader(inFile);
        Locator locator;
        if (loaderRequestIsDone() || allLoopIsDone())
            return;
        locator.locateFromBegin(inFile, cursor.nextReadPos(header.directoryOffset)); // 定位到未读目录块

        try
        {
            // 读取Header
            if (!isReadHeader)
            {
                loadHeaderStandard(inFile, header, buffer);
                isReadHeader = true;
            }

            if (cursor.onlyMagicRemains())
            {
                Y_flib::SizeOfMagicNum magicNum = standardsReader.readBinaryStandards<Y_flib::SizeOfMagicNum>();
                if (magicNum != Y_flib::Constants::MAGIC_NUM)
                    throw std::runtime_error("Invalid MAGIC_NUM");

                setAllLoopDone(); // 完成所有循环后设置完成标志并return退出
                return;
            }
            while (cursor.hasData())
            {
                if (!cursor.hasData())
                    break;
                if (loaderRequestIsDone() || allLoopIsDone())
                    return;

                buffer.clear();
                loadEntryBlock(standardsReader, countOfChildDirectory, encryption);
            }
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("BinaryStandardLoader encountered an error: ") + e.what());
        }
    }

    void BinaryStandardLoader::loadEntryBlock(StandardsReader &standardsReader, Y_flib::FileCount &countOfChildDirectory, Y_flib::IEncryption &encryption)
    {
        if (!cursor.hasData())
            return;

        Y_flib::FlagType flag;
        Y_flib::IvSize ivNum{};
        loadSeparatedStandard(const_cast<Y_flib::FlagType &>(flag), standardsReader, ivNum);

        // 读取加密数据到vector，等待解密处理：将读取到的数据块位置信息存入队列，供后续加密使用
        Y_flib::BlockLength readSize = cursor.bytesInThisBlock();

        blockPosition.push_back(BlockSpan{
            static_cast<Y_flib::SlotOffset>(inFile.tellg()), // 块数据起始绝对偏移
            readSize                                          // 块字节数
        }); // 记录数据块位置信息，供后续加密操作使用

        // 根据偏移量读取数据块
        StandardsReader::readDataBlock(readSize, inFile, buffer);

        if (std::any_of(ivNum.begin(), ivNum.end(), [](auto byte)
                        { return byte != 0; })) // 当需要解压时，对buffer进行解密操作
        {
            Y_flib::DataBlock blockWithIv;
            Y_flib::DataBlock decryptedBlock;
            blockWithIv.resize(sizeof(Y_flib::IvSize));
            std::memcpy(blockWithIv.data(), &ivNum, sizeof(Y_flib::IvSize));
            blockWithIv.insert(blockWithIv.end(), buffer.begin(), buffer.end());

            encryption.decrypt(blockWithIv, decryptedBlock);
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
                if (!directoryQueueReady.empty())
                {
                    if (directoryQueueReady.back() != directoryPath)
                    {
                        directoryQueueReady.push(parentPath / directoryPath);
                    }
                }
                else if (firstReady) // 用firstReady让第一个元素入队，避免队列判空出问题
                {
                    directoryQueueReady.push(parentPath / directoryPath);
                    firstReady = false;
                }

                entryQueue.pop();
                if (!entryQueue.empty())
                {
                    countOfChildDirectory = entryQueue.front().second; // 获取子目录数量
                    if (!entryQueue.empty())
                        directoryQueueReady.push(entryQueue.front().first.getFullPath()); // pop前将当前目录加入，确保完整性
                }
            }
        }
        setRequestDone();            // 设置块完成标志
        if (cursor.isLastBlock())    // blockLen 为 0 说明是末块，读完后扣减剩余量
        {
            cursor.consumeFinalBlock(readSize);
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
            // v2 增加 Windows 链接类型；不含链接的 v1 普通归档继续兼容读取。
            if (header.version < Y_flib::Constants::MIN_SUPPORTED_VERSION ||
                header.version > Y_flib::Constants::VERSION)
            {
                throw std::runtime_error("Unsupported archive version: supported " +
                                         std::to_string(Y_flib::Constants::MIN_SUPPORTED_VERSION) +
                                         "-" + std::to_string(Y_flib::Constants::VERSION) +
                                         ", got " + std::to_string(header.version));
            }
            if (header.directoryOffset == 0)
                throw std::runtime_error("Invalid directory offset in header");
            cursor.initFromHeader(header.directoryOffset); // remaining = 数据区起点 - 文件头
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

        // 读取本块长度，并跳过分割标准槽位与块体（0 表示最后一块，长度由剩余量推出）
        Y_flib::BlockLength blockLen = standardsReader.readBinaryStandards<Y_flib::BlockLength>();
        // 读取iv头
        ivNum = standardsReader.readBinaryStandards<Y_flib::IvSize>();

        cursor.consumeSeparated(blockLen);
        if (flag != Y_flib::FlagType::Separated)
        {
            throw std::runtime_error("Invalid flag type for separated standard");
        }
    }

    void BinaryStandardLoader::setRequestDone()
    {

        blockIsDone = true;
    }
    void BinaryStandardLoader::setAllLoopDone()
    {
        // 不在这里关闭文件流，让它们保持打开状态
        // 文件流会在析构函数中自动关闭
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

            Y_flib::SlotOffset offsetToRestart = cursor.nextReadPos(header.directoryOffset);

            locator.locateFromBegin(newInFile, offsetToRestart);
            this->inFile = std::move(newInFile);
            blockIsDone = false;
        }
    }
} // namespace Y_flib
