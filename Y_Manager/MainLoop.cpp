#include "MainLoop.h"

void CompressionLoop ::compressionLoop(const std::vector<std::string> &filePathToScan, Aes &aes)
{
    // 初始化迭代器
    std::filesystem::path blank;
    BinaryStandardLoader headerLoaderIterator(compressionFilePath, filePathToScan, blank);

    Heffman huffmanZip(1);

    PathTransfer transfer;

    Y_flib::DataBlock encryptedBlock;

    std::filesystem::path loadPath;
    std::unique_ptr<DataLoader> dataLoader;

    Y_flib::FileSize totalBlocks = 1, blockCount = 0;

    headerLoaderIterator.headerLoaderIterator(aes); // 执行第一次操作，把根目录载入
    if (!headerLoaderIterator.fileQueue.empty())    // 单个文件特殊处理
    {

        EntryDetails loadFile = headerLoaderIterator.fileQueue.front().first;
        loadPath = loadFile.getFullPath();
        dataLoader = std::make_unique<DataLoader>(loadPath);
        totalBlocks = (loadFile.getFileSizeInDetails() + Y_flib::Constants::BUFFER_SIZE - 1) / Y_flib::Constants::BUFFER_SIZE;
    }

    DataExporter dataExporter(transfer.transPath(compressionFilePath));

    std::filesystem::path filename = loadPath.filename();
    while (!headerLoaderIterator.fileQueue.empty())
    {

        dataLoader->dataLoader();

        if ((!dataLoader->isDone() && blockCount < totalBlocks)) // 处理当前文件的每个数据块
        {
            blockCount++;
            const Y_flib::DataBlock data_In = dataLoader->getBlock(); // 获取一次数据块，重复使用

            huffmanZip.statistic_freq(0, data_In);

            huffmanZip.merge_ttabs();
            huffmanZip.gen_hefftree();
            huffmanZip.save_code_inTab();
            Y_flib::DataBlock huffTree;
            huffmanZip.tree_to_plat_uchar(huffTree);
            Y_flib::DataBlock huffTreeOutPut(huffTree.size());

            aes.doAes(1, huffTree, huffTreeOutPut);
            dataExporter.exportCompressedData(huffTreeOutPut);

            // system("cls");

            Y_flib::DataBlock compressedData;
            huffmanZip.encode(data_In, compressedData);

            aes.doAes(1, compressedData, encryptedBlock);

            dataExporter.exportCompressedData(encryptedBlock); // 读取的数据传输给exporter
            encryptedBlock.clear();

            std::cout << "Processing file: " << filename << "\n"
                      << std::fixed << std::setw(6) << std::setprecision(2)
                      << (100.0 * blockCount) / totalBlocks
                      << "% \n";
        }

        if (dataLoader->isDone() && !headerLoaderIterator.fileQueue.empty()) // 当前文件处理完成，准备下一个文件
        {
            Y_flib::FileNameSize offsetToFill = headerLoaderIterator.fileQueue.front().second;
            dataExporter.thisFileIsDone(offsetToFill);

            headerLoaderIterator.fileQueue.pop();

            if (!headerLoaderIterator.fileQueue.empty())
            { // 更新下一个文件路径
                EntryDetails newLoadFile = headerLoaderIterator.fileQueue.front().first;
                dataLoader->reset(newLoadFile.getFullPath());
                filename = newLoadFile.getFullPath().filename();
                totalBlocks = (newLoadFile.getFileSizeInDetails() + Y_flib::Constants::BUFFER_SIZE - 1) / Y_flib::Constants::BUFFER_SIZE;
                blockCount = 0;
            }
        }
        while (headerLoaderIterator.fileQueue.empty() && !headerLoaderIterator.allLoopIsDone()) // 队列空但整体未完成，循环请求读取对队列进行填充，直到符合条件为止
        {
            // 重启迭代器并且请求填充下一轮队列
            headerLoaderIterator.restartLoader();
            headerLoaderIterator.headerLoaderIterator(aes);

            if (!headerLoaderIterator.fileQueue.empty())
            { // 更新下一个文件路径(每块第一个文件)
                EntryDetails newLoadFile = headerLoaderIterator.fileQueue.front().first;
                dataLoader->reset(newLoadFile.getFullPath());
                filename = newLoadFile.getFullPath().filename();
                totalBlocks = (newLoadFile.getFileSizeInDetails() + Y_flib::Constants::BUFFER_SIZE - 1) / Y_flib::Constants::BUFFER_SIZE;
                blockCount = 0;
            }
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
            std::filesystem::path dirToCreate = headerLoaderIterator.directoryQueue_ready.front();

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
            std::filesystem::path relativePath = headerLoaderIterator.fileQueue.front().first.getFullPath();
            std::filesystem::path fullFilePath = parentPath / relativePath;

            createFile(fullFilePath); // 重建文件
            // 获取当前的 inFile 引用（在每个文件处理前重新获取，以避免悬挂引用）
            std::ifstream &inFile = headerLoaderIterator.getInFile();
            // 把已压缩块读进内存，处理，写入对应位置

            locator.locateFromBegin(inFile, dataOffset); // 定位到数据区（或已处理块后）

            std::filesystem::path filePath = fullFilePath;
            std::filesystem::path filename = filePath.filename();
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

                // 在每次循环中重新创建 StandardsReader 以确保正确读取
                StandardsReader numReader(inFile);
                // 循环中创建loader对象，使用当前文件路径，确保每次读取都能正确关联到当前文件
                DataLoader loader(filePath);

                // 读取分隔标志
                if (!(numReader.readBinaryStandards<Y_flib::FlagType>() == Y_flib::FlagType::Separated))
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
                if (!(numReader.readBinaryStandards<Y_flib::FlagType>() == Y_flib::FlagType::Separated))
                    throw std::runtime_error("decompressionLoop()-Error:Can't read SEPARATED_FLAG before data block");
                // 读取数据块大小
                Y_flib::DirectoryOffsetSize blockSize = numReader.readBinaryStandards<Y_flib::DirectoryOffsetSize>();
                // 读取加密的压缩数据
                Y_flib::DataBlock rawData(blockSize);

                loader.dataLoader(blockSize, inFile, rawData);

                Y_flib::FileSize readedSize = inFile.gcount();
                if (readedSize != blockSize)
                {
                    throw std::runtime_error(
                        "decompressionLoop()-Error: Failed to read complete data block. Expected " +
                        std::to_string(blockSize) +
                        " bytes, got " +
                        std::to_string(readedSize));
                }
                // 解密数据(doAes会重新分配outputBuffer,不需要预先指定大小)
                Y_flib::DataBlock decryptedData;
                aes.doAes(2, rawData, decryptedData);
                // 使用该块对应的 Huffman 树进行解压
                Y_flib::FileSize remainingBytes = originalSize - totalDecompressedBytes;
                Y_flib::DataBlock decompressedData;
                huffmanUnzip.decode(decryptedData, decompressedData, BitHandler(), remainingBytes);
                // 更新已解压的总字节数
                totalDecompressedBytes += decompressedData.size();
                // 写入解压后的数据
                dataExporter.exportDecompressedData(decompressedData);

                // 更新剩余大小: 减去压缩数据块本身的大小
                // 注意: FLAG 和 size 字段不计入 fileCompressedSize
                fileCompressedSize -= blockSize;
            }
            // 更新dataOffset为下一个文件的起始位置
            dataOffset = inFile.tellg();

            headerLoaderIterator.fileQueue.pop();
        }
        while (headerLoaderIterator.fileQueue.empty() && !headerLoaderIterator.allLoopIsDone()) // 队列空但整体未完成，循环请求读取对队列进行填充，直到符合条件为止
        {
            headerLoaderIterator.restartLoader();
            headerLoaderIterator.headerLoaderIterator(aes);
        }
    }
}
void DecompressionLoop::createDirectory(const std::filesystem::path &directoryPath)
{
    try
    {
        // 确保父目录存在
        if (!directoryPath.parent_path().empty() && !std::filesystem::exists(directoryPath.parent_path()))
        {
            createDirectory(directoryPath.parent_path());
        }

        bool created = std::filesystem::create_directory(directoryPath);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("createDirectory()-Error: " + directoryPath.string());
    }
}

// 创建文件 (创建空文件)
bool DecompressionLoop::createFile(const std::filesystem::path &filePath)
{
    try
    {
        if (std::filesystem::exists(filePath))
        {
            std::cerr << "fileIsExist: " << filePath << " ,skipped to next \n";
            return false; // 文件已存在，跳过创建
        }

        // 确保父目录存在
        if (!filePath.parent_path().empty() && !std::filesystem::exists(filePath.parent_path()))
        {
            createDirectory(filePath.parent_path());
        }

        std::ofstream outfile(filePath);
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        throw std::runtime_error("createDirectory()-Error: " + filePath.string());
    }
    return true;
}
