#include "../include/HeaderLoader.h"

void HeaderLoader_Compression ::headerLoader(const std::string compressionFilePath, const std::vector<std::string> &filePathToScan, Aes &aes)
{
    // 初始化加载器
    BinaryIO_Loader headerLoader(compressionFilePath, filePathToScan);
    DataBlock encryptedBlock;
    FileSize_uint totalBlocks = 1, count = 0;
    headerLoader.headerLoader();         // 执行第一次操作，把根目录载入
    if (!headerLoader.fileQueue.empty()) // 单个文件特殊处理
    {
        Directory_FileDetails &loadFile = headerLoader.fileQueue.front().first;
        loadPath = loadFile.getFullPath();
        dataLoader = new DataLoader(loadPath);
        totalBlocks = (loadFile.getFileSize() + BUFFER_SIZE - 1) / BUFFER_SIZE;
    }

    DataExporter dataExporter(transfer.transPath(compressionFilePath));

    fs::path filename = loadPath.filename();
    while (!headerLoader.fileQueue.empty())
    {

        dataLoader->dataLoader();

        if (!dataLoader->isDone()) // 避免读到空数据块
        {
            system("cls");
            std::cout << "Processing file: " << filename << "\n"
                      << std::fixed << std::setw(6) << std::setprecision(2)
                      << (100.0 * ++count) / totalBlocks
                      << "% \n";
            encryptedBlock.resize(BUFFER_SIZE + sizeof(IvSize_uint));
            aes.doAes(1, dataLoader->getBlock(), encryptedBlock);
            dataExporter.exportDataToFile_Encryption(encryptedBlock); // 读取的数据传输给exporter
            encryptedBlock.clear();
        }
        else if (dataLoader->isDone() && !headerLoader.fileQueue.empty())
        {
            FileNameSize_uint offsetToFill = headerLoader.fileQueue.front().second;
            dataExporter.thisFileIsDone(offsetToFill); // 可在此前插入一个编码表写入再调用done
            std::cout << "--------Done!--------" << "\n";

            headerLoader.fileQueue.pop();
            if (!headerLoader.fileQueue.empty())
            { // 更新下一个文件路径，生成编码表时可以调用reset（原目录）读两轮
                Directory_FileDetails &loadFile = headerLoader.fileQueue.front().first;
                dataLoader->reset(loadFile.getFullPath());
                filename = loadFile.getFullPath().filename();
                totalBlocks = (loadFile.getFileSize() + BUFFER_SIZE - 1) / BUFFER_SIZE;
                count = 0;
            }
        }

        if (headerLoader.fileQueue.empty() && !headerLoader.allLoopIsDone()) // 队列空但整体未完成，请求下一轮读取对队列进行填充
        {
            headerLoader.restartLoader();
            headerLoader.headerLoader();
        }
    }
}

void BinaryIO_Loader::headerLoader()
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
            loadBySepratedFlag(numsReader, countOfKidDirectory);
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

        while (readSize > bufferPtr)
        {

            while ((countOfKidDirectory > 0 || bufferPtr == 0) && readSize > bufferPtr)
            {
                parserForLoader->parser(bufferPtr, filePathToScan, countOfKidDirectory);
            }

            if (!directoryQueue.empty() && countOfKidDirectory == 0)
            {

                directoryQueue.pop();
                if (!directoryQueue.empty())
                    countOfKidDirectory = directoryQueue.front().second;
            }
        }
        requesetDone();
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