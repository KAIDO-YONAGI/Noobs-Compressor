#include "MainLoop.h"

void CompressionLoop ::compressionLoop(const std::vector<std::string> &filePathToScan, Aes &aes)
{
    // 初始化迭代器
    fs::path blank;
    BinaryIO_Loader headerLoaderIterator(compressionFilePath, filePathToScan, blank);
    Heffman huffmanZip(1);
    DataBlock encryptedBlock;
    FileSize_uint totalBlocks = 1, count = 0;
    fs::path loadPath;
    std::unique_ptr<DataLoader> dataLoader;

    headerLoaderIterator.headerLoaderIterator(aes); // 执行第一次操作，把根目录载入
    if (!headerLoaderIterator.fileQueue.empty())    // 单个文件特殊处理
    {
        Directory_FileDetails loadFile = headerLoaderIterator.fileQueue.front().first;
        loadPath = loadFile.getFullPath();
        dataLoader = std::make_unique<DataLoader>(loadPath);
        totalBlocks = (loadFile.getFileSize() + BUFFER_SIZE - 1) / BUFFER_SIZE;
    }

    DataExporter dataExporter(transfer.transPath(compressionFilePath));

    fs::path filename = loadPath.filename();
    while (!headerLoaderIterator.fileQueue.empty())
    {

        dataLoader->dataLoader();
        if (!dataLoader->isDone())
        {
            huffmanZip.statistic_freq(0, dataLoader->getBlock());

            huffmanZip.merge_ttabs();
            huffmanZip.gen_hefftree();
            huffmanZip.save_code_inTab();
            DataBlock huffTree;
            huffmanZip.tree_to_plat_uchar(huffTree);
            DataBlock huffTree_outPut(huffTree.size());

            aes.doAes(1, huffTree, huffTree_outPut);
            dataExporter.exportDataToFile_Compression(huffTree_outPut);
        }
        if (count < totalBlocks)
        {
            count++;
            dataLoader->resetByLastReaded();

            system("cls");
            std::cout << "Processing file: " << filename << "\n"
                      << std::fixed << std::setw(6) << std::setprecision(2)
                      << (100.0 * count) / totalBlocks
                      << "% \n";

            DataBlock data_In = dataLoader->getBlock(); // 调用压缩
            DataBlock compressedData;
            huffmanZip.encode(data_In, compressedData);

            aes.doAes(1, compressedData, encryptedBlock);

            dataExporter.exportDataToFile_Compression(encryptedBlock); // 读取的数据传输给exporter
            encryptedBlock.clear();
        }

        if (dataLoader->isDone() && !headerLoaderIterator.fileQueue.empty())
        {
            FileNameSize_uint offsetToFill = headerLoaderIterator.fileQueue.front().second;
            dataExporter.thisFileIsDone(offsetToFill);
            std::cout << "--------Done!--------" << "\n";

            headerLoaderIterator.fileQueue.pop();
            if (!headerLoaderIterator.fileQueue.empty())
            { // 更新下一个文件路径
                Directory_FileDetails newLoadFile = headerLoaderIterator.fileQueue.front().first;
                dataLoader->reset(newLoadFile.getFullPath());
                filename = newLoadFile.getFullPath().filename();
                totalBlocks = (newLoadFile.getFileSize() + BUFFER_SIZE - 1) / BUFFER_SIZE;
                count = 0;
            }
        }

        if (headerLoaderIterator.fileQueue.empty() && !headerLoaderIterator.allLoopIsDone()) // 队列空但整体未完成，请求下一轮读取对队列进行填充
        {
            headerLoaderIterator.restartLoader();
            headerLoaderIterator.headerLoaderIterator(aes);
        }
    }
    headerLoaderIterator.encryptHeaderBlock(aes); // 加密目录块并且回填
}

void DecompressionLoop::decompressionLoop(Aes &aes)
{

    std::vector<std::string> blank;
    BinaryIO_Loader headerLoaderIterator(fullPath.string(), blank, parentPath);

    std::unique_ptr<DataLoader> dataLoader = std::make_unique<DataLoader>();

    headerLoaderIterator.headerLoaderIterator(aes); // 执行第一次操作，把根目录载入
    DirectoryOffsetSize_uint dataOffset = headerLoaderIterator.getDirectoryOffset();

    // 创建 Huffman 对象用于解压
    Heffman huffmanUnzip(1);
    bool treeLoaded = false;

    while (!headerLoaderIterator.allLoopIsDone())
    {
        while (!headerLoaderIterator.directoryQueue_ready.empty()) // 重建目录
        {
            createDirectory(headerLoaderIterator.directoryQueue_ready.front());
            headerLoaderIterator.directoryQueue_ready.pop();
        }
        while (!headerLoaderIterator.fileQueue.empty())
        {
            createFile(headerLoaderIterator.fileQueue.front().first.getFullPath()); // 重建文件

            // headerLoaderIterator.fileQueue.pop();
            // continue;

            // 获取当前的 inFile 引用（在每个文件处理前重新获取，以避免悬挂引用）
            std::ifstream &inFile = headerLoaderIterator.getInFile();
            NumsReader numReader(inFile); // 在此处创建 NumsReader，确保引用有效

            // 把已压缩块读进内存，处理，写入对应位置
            inFile.clear();                          // 清除可能的错误标志(如eof)，确保seek可以正常工作
            inFile.seekg(dataOffset, std::ios::beg); // 定位到数据区（或已处理块后）
            if (!inFile.good())
            {
                throw std::runtime_error("decompressionLoop()-Error:Failed to seek to dataOffset " + std::to_string(dataOffset));
            }
            if (!(numReader.readBinaryNums<char>() == SEPARATED_FLAG)) // 检测分割标志
                throw std::runtime_error("decompressionLoop()-Error:Can't read SEPARATED_FLAG");
            fs::path filePath = headerLoaderIterator.fileQueue.front().first.getFullPath();
            fs::path filename = filePath.filename();

            system("cls");
            std::cout << "Processing file: " << filename << "\n";

            DataExporter dataExporter(filePath);
            FileSize_uint fileCompressedSize = headerLoaderIterator.fileQueue.front().second;
            bool isFirstBlock = true;

            // 读取并恢复 Huffman 树（每个文件第一个块是树数据）
            if (!treeLoaded)
            {
                DirectoryOffsetSize_uint treeBlockSize = numReader.readBinaryNums<DirectoryOffsetSize_uint>();
                dataLoader->dataLoader(treeBlockSize, inFile);
                DataBlock rawTreeData = dataLoader->getBlock();

                // 解密树数据
                DataBlock decryptedTreeData(rawTreeData.size() + sizeof(IvSize_uint));
                aes.doAes(2, rawTreeData, decryptedTreeData);

                // 恢复 Huffman 树（树数据是序列化的文本格式，不需要解压）
                huffmanUnzip.spawn_tree(decryptedTreeData);
                treeLoaded = true;

                fileCompressedSize -= treeBlockSize;
                dataOffset += treeBlockSize + SEPARATED_STANDARD_SIZE - sizeof(IvSize_uint);
                isFirstBlock = true;
            }

            while (fileCompressedSize > 0)
            {
                // 每个块前面都有 SEPARATED_FLAG，第一个块已经在循环外读过了
                if (!isFirstBlock)
                {
                    if (!(numReader.readBinaryNums<char>() == SEPARATED_FLAG))
                        throw std::runtime_error("decompressionLoop()-Error:Can't read SEPARATED_FLAG in loop");
                }
                isFirstBlock = false;

                DirectoryOffsetSize_uint blockSize = numReader.readBinaryNums<DirectoryOffsetSize_uint>();

                DirectoryOffsetSize_uint readSize = (blockSize != 0 ? blockSize : fileCompressedSize);

                dataLoader->dataLoader(readSize, inFile);

                DataBlock rawData = dataLoader->getBlock(); // deAes
                DataBlock decryptedData(rawData.size() + sizeof(IvSize_uint));
                aes.doAes(2, rawData, decryptedData);

                // 进行 Huffman 解压
                DataBlock decompressedData;
                BitHandler bitHandler;
                bitHandler.bytecount = decryptedData.size(); // 设置总字节数
                huffmanUnzip.decode(decryptedData, decompressedData, bitHandler);

                // 写入解压后的数据
                dataExporter.exportDataToFile_Decompression(decompressedData);

                fileCompressedSize -= readSize;
                dataOffset += readSize + SEPARATED_STANDARD_SIZE - sizeof(IvSize_uint); // 更新数据区位置
            }
            std::cout << "--------Done!--------" << "\n";

            headerLoaderIterator.fileQueue.pop();
            treeLoaded = false; // 重置树加载标志，为下一个文件做准备
        }

        if (headerLoaderIterator.fileQueue.empty() && !headerLoaderIterator.allLoopIsDone()) // 队列空但整体未完成，请求下一轮读取对队列进行填充
        {
            headerLoaderIterator.restartLoader();
            headerLoaderIterator.headerLoaderIterator(aes);
        }
    }
}
void DecompressionLoop::createDirectory(const fs::path &directoryPath)
{
    try
    {
        if (fs::exists(directoryPath))
        {
            std::cout << "directoryIsExist: " << directoryPath << " ,skipped to next \n"; // 可以优化用户选择覆盖机制
        }
        bool created = fs::create_directory(directoryPath);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("createDirectory()-Error: " + directoryPath.string());
    }
}

// 创建文件 (创建空文件)
void DecompressionLoop::createFile(const fs::path &filePath)
{
    try
    {
        if (fs::exists(filePath))
        {
            std::cerr << "fileIsExist: " << filePath << " ,skipped to next \n";
        }

        // 确保父目录存在
        if (!filePath.parent_path().empty() && !fs::exists(filePath.parent_path()))
        {
            createDirectory(filePath.parent_path());
        }

        std::ofstream outfile(filePath);
    }
    catch (const fs::filesystem_error &e)
    {
        throw std::runtime_error("createDirectory()-Error: " + filePath.string());
    }
}
