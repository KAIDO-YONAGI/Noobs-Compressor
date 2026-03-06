#include "MainLoop.h"
namespace fs = std::filesystem;

void CompressionLoop ::compressionLoop(const std::vector<std::string> &filePathToScan, Aes &aes)
{
    // 初始化迭代器
    fs::path blank;
    BinaryStandardLoader headerLoaderIterator(compressionFilePath, filePathToScan, blank);

    Heffman huffmanZip(1);

    PathTransfer transfer;

    Y_flib::DataBlock encryptedBlock;

    fs::path loadPath;
    std::unique_ptr<DataLoader> dataLoader;

    Y_flib::FileSize totalBlocks = 1, count = 0;
    headerLoaderIterator.headerLoaderIterator(aes); // 执行第一次操作，把根目录载入
    if (!headerLoaderIterator.fileQueue.empty())    // 单个文件特殊处理
    {
        Directory_FileDetails loadFile = headerLoaderIterator.fileQueue.front().first;
        loadPath = loadFile.getFullPath();
        dataLoader = std::make_unique<DataLoader>(loadPath);
        totalBlocks = (loadFile.getFileSizeInDetails() + BUFFER_SIZE - 1) / BUFFER_SIZE;
    }

    DataExporter dataExporter(transfer.transPath(compressionFilePath));

    fs::path filename = loadPath.filename();
    while (!headerLoaderIterator.fileQueue.empty())
    {

        dataLoader->dataLoader();
        if (!dataLoader->isDone() && count < totalBlocks)
        {
            count++;
            const Y_flib::DataBlock data_In = dataLoader->getBlock(); // 获取一次数据块，重复使用
            huffmanZip.statistic_freq(0, data_In);

            huffmanZip.merge_ttabs();
            huffmanZip.gen_hefftree();
            huffmanZip.save_code_inTab();
            Y_flib::DataBlock huffTree;
            huffmanZip.tree_to_plat_uchar(huffTree);
            Y_flib::DataBlock huffTree_outPut(huffTree.size());

            aes.doAes(1, huffTree, huffTree_outPut);
            dataExporter.exportDataToFile_Compression(huffTree_outPut);

            system("cls");

            std::cout << "Processing file: " << filename << "\n"
                      << std::fixed << std::setw(6) << std::setprecision(2)
                      << (100.0 * count) / totalBlocks
                      << "% \n";

            Y_flib::DataBlock compressedData;
            huffmanZip.encode(data_In, compressedData);

            aes.doAes(1, compressedData, encryptedBlock);

            dataExporter.exportDataToFile_Compression(encryptedBlock); // 读取的数据传输给exporter
            encryptedBlock.clear();
        }

        if (dataLoader->isDone() && !headerLoaderIterator.fileQueue.empty())
        {
            Y_flib::FileNameSize offsetToFill = headerLoaderIterator.fileQueue.front().second;
            dataExporter.thisFileIsDone(offsetToFill);

            headerLoaderIterator.fileQueue.pop();
            if (!headerLoaderIterator.fileQueue.empty())
            { // 更新下一个文件路径
                Directory_FileDetails newLoadFile = headerLoaderIterator.fileQueue.front().first;
                dataLoader->reset(newLoadFile.getFullPath());
                filename = newLoadFile.getFullPath().filename();
                totalBlocks = (newLoadFile.getFileSizeInDetails() + BUFFER_SIZE - 1) / BUFFER_SIZE;
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
    BinaryStandardLoader headerLoaderIterator(fullPath.string(), blank, parentPath);
    headerLoaderIterator.headerLoaderIterator(aes); // 执行第一次操作，把根目录载入
    Y_flib::DirectoryOffsetSize dataOffset = headerLoaderIterator.getDirectoryOffset();
    // 创建 Huffman 对象用于解压
    Heffman huffmanUnzip(1);
    Locator locator;

    while (!headerLoaderIterator.allLoopIsDone())
    {
        while (!headerLoaderIterator.directoryQueue_ready.empty()) // 重建目录
        {
            fs::path dirToCreate = headerLoaderIterator.directoryQueue_ready.front();

            // 如果是相对路径，拼接parentPath
            if (!dirToCreate.is_absolute())
            {
                dirToCreate = parentPath / dirToCreate;
            }

            createDirectory(dirToCreate);
            headerLoaderIterator.directoryQueue_ready.pop();
        }
        while (!headerLoaderIterator.fileQueue.empty())
        {
            // 获取相对路径并拼接 parentPath
            fs::path relativePath = headerLoaderIterator.fileQueue.front().first.getFullPath();
            fs::path fullFilePath = parentPath / relativePath;

            createFile(fullFilePath); // 重建文件
            // 获取当前的 inFile 引用（在每个文件处理前重新获取，以避免悬挂引用）
            std::ifstream &inFile = headerLoaderIterator.getInFile();
            // 把已压缩块读进内存，处理，写入对应位置
            inFile.clear();                              // 清除可能的错误标志(如eof)，确保seek可以正常工作
            locator.locateFromBegin(inFile, dataOffset); // 定位到数据区（或已处理块后）

            fs::path filePath = fullFilePath;
            fs::path filename = filePath.filename();
            // system("cls");
            std::cout << "Processing file: " << filename << "\n";
            DataExporter dataExporter(filePath);
            Y_flib::FileSize fileCompressedSize = headerLoaderIterator.fileQueue.front().second;
            // 获取原始文件大小（未压缩的大小）用于控制解压输出
            Y_flib::FileSize originalSize = headerLoaderIterator.fileQueue.front().first.getFileSizeInDetails();
            Y_flib::FileSize totalDecompressedBytes = 0; // 跟踪已经解压的总字节数

            // 处理文件的每个块：每个块都有独立的 Huffman 树
            while (totalDecompressedBytes < originalSize && fileCompressedSize > 0)
            {

                // 在每次循环中重新创建 BinaryStandardsReader 以确保正确读取
                BinaryStandardsReader numReader(inFile);
                //循环中创建loader对象，使用当前文件路径，确保每次读取都能正确关联到当前文件
                DataLoader loader(filePath);

                // 读取分隔标志
                if (!(numReader.readBinaryStandards<char>() == SEPARATED_FLAG))
                    throw std::runtime_error("decompressionLoop()-Error:Can't read SEPARATED_FLAG before tree block");

                // 读取 Huffman 树块大小
                std::streampos treeSizePos = inFile.tellg();
                Y_flib::DirectoryOffsetSize treeBlockSize = numReader.readBinaryStandards<Y_flib::DirectoryOffsetSize>();
                // 读取并解密树数据
                Y_flib::DataBlock rawTreeData(treeBlockSize);


                loader.dataLoader(treeBlockSize, inFile, rawTreeData);

                std::streamsize bytesRead = inFile.gcount();
                Y_flib::DataBlock decryptedTreeData;
                aes.doAes(2, rawTreeData, decryptedTreeData);
                // 恢复 Huffman 树（为这个块创建新树）
                huffmanUnzip.spawn_tree(decryptedTreeData);
                // 检查树是否正确构建
                if (huffmanUnzip.getTreeRoot() == nullptr)
                {
                    throw std::runtime_error("decompressionLoop()-Error: Failed to spawn Huffman tree - tree root is NULL");
                }
                // FLAG 和 size 字段不计入 fileCompressedSize
                fileCompressedSize -= treeBlockSize;
                // 读取分割标志(数据块前的标志)
                if (!(numReader.readBinaryStandards<char>() == SEPARATED_FLAG))
                    throw std::runtime_error("decompressionLoop()-Error:Can't read SEPARATED_FLAG before data block");
                // 读取数据块大小
                Y_flib::DirectoryOffsetSize blockSize = numReader.readBinaryStandards<Y_flib::DirectoryOffsetSize>();
                // 读取加密的压缩数据
                Y_flib::DataBlock rawData(blockSize);

                loader.dataLoader(blockSize, inFile, rawData);

                std::streamsize readedSize = inFile.gcount();
                if (readedSize != static_cast<std::streamsize>(blockSize))
                {
                    throw std::runtime_error("decompressionLoop()-Error: Failed to read complete data block. Expected " +
                                             std::to_string(blockSize) + " bytes, got " + std::to_string(readedSize));
                }
                // 解密数据(doAes会重新分配outputBuffer,不需要预先指定大小)
                Y_flib::DataBlock decryptedData;
                aes.doAes(2, rawData, decryptedData);
                // 使用该块对应的 Huffman 树进行解压
                size_t remainingBytes = originalSize - totalDecompressedBytes;
                size_t maxBytesThisBlock = std::min(remainingBytes, (size_t)BUFFER_SIZE);
                Y_flib::DataBlock decompressedData;
                huffmanUnzip.decode(decryptedData, decompressedData, BitHandler(), maxBytesThisBlock);
                // 更新已解压的总字节数
                totalDecompressedBytes += decompressedData.size();
                // 写入解压后的数据
                dataExporter.exportDataToFile_Decompression(decompressedData);

                // 更新剩余大小: 减去压缩数据块本身的大小
                // 注意: FLAG 和 size 字段不计入 fileCompressedSize
                fileCompressedSize -= blockSize;
            }
            // 更新dataOffset为下一个文件的起始位置
            dataOffset = inFile.tellg();

            headerLoaderIterator.fileQueue.pop();
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
        // 确保父目录存在
        if (!directoryPath.parent_path().empty() && !fs::exists(directoryPath.parent_path()))
        {
            createDirectory(directoryPath.parent_path());
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
