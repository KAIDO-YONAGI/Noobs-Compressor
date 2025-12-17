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
        if (!dataLoader->isDone() && count < totalBlocks)
        {
            count++;
            huffmanZip.statistic_freq(0, dataLoader->getBlock());

            huffmanZip.merge_ttabs();
            huffmanZip.gen_hefftree();
            huffmanZip.save_code_inTab();
            DataBlock huffTree;
            huffmanZip.tree_to_plat_uchar(huffTree);
            std::cout << "[TREE] Block #" << count << " tree size: " << huffTree.size() << " bytes\n";
            DataBlock huffTree_outPut(huffTree.size());

            aes.doAes(1, huffTree, huffTree_outPut);
            std::cout << "[TREE] Encrypted tree size: " << huffTree_outPut.size() << " bytes\n";
            dataExporter.exportDataToFile_Compression(huffTree_outPut);

            // system("cls");
            if(filename=="ObjectIdentifier.js")
            int a=1;
            std::cout << "Processing file: " << filename << "\n"
                      << std::fixed << std::setw(6) << std::setprecision(2)
                      << (100.0 * count) / totalBlocks
                      << "% \n";

            DataBlock data_In = dataLoader->getBlock(); // 调用压缩
            std::cout << "[COMPRESS] Block #" << count << " loaded " << data_In.size() << " bytes from file\n";

            DataBlock compressedData;
            huffmanZip.encode(data_In, compressedData);
            std::cout << "[COMPRESS] Block #" << count << " compressed to " << compressedData.size() << " bytes\n";

            aes.doAes(1, compressedData, encryptedBlock);
            std::cout << "[COMPRESS] Block #" << count << " encrypted to " << encryptedBlock.size() << " bytes\n";

            dataExporter.exportDataToFile_Compression(encryptedBlock); // 读取的数据传输给exporter
            encryptedBlock.clear();
        }

        if (dataLoader->isDone() && !headerLoaderIterator.fileQueue.empty())
        {
            FileNameSize_uint offsetToFill = headerLoaderIterator.fileQueue.front().second;
            dataExporter.thisFileIsDone(offsetToFill);
            std::cout << "\n========================================\n";
            std::cout << "File compression complete: " << filename << "\n";
            std::cout << "Total blocks: " << count << "\n";
            std::cout << "========================================\n\n";

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

    headerLoaderIterator.headerLoaderIterator(aes); // 执行第一次操作，把根目录载入
    DirectoryOffsetSize_uint dataOffset = headerLoaderIterator.getDirectoryOffset();
    std::cout << "[DECOMPRESS] Initial dataOffset: " << dataOffset << "\n";

    // 创建 Huffman 对象用于解压
    Heffman huffmanUnzip(1);

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

            // 获取当前的 inFile 引用（在每个文件处理前重新获取，以避免悬挂引用）
            std::ifstream &inFile = headerLoaderIterator.getInFile();

            // 把已压缩块读进内存，处理，写入对应位置
            inFile.clear();                          // 清除可能的错误标志(如eof)，确保seek可以正常工作
            std::cout << "[DECOMPRESS] Before seek: current position=" << inFile.tellg() << ", seeking to dataOffset=" << dataOffset << "\n";
            inFile.seekg(dataOffset, std::ios::beg); // 定位到数据区（或已处理块后）
            if (!inFile.good())
            {
                throw std::runtime_error("decompressionLoop()-Error:Failed to seek to dataOffset " + std::to_string(dataOffset));
            }
            std::cout << "[DECOMPRESS] After seek: current position=" << inFile.tellg() << "\n";

            fs::path filePath = headerLoaderIterator.fileQueue.front().first.getFullPath();
            fs::path filename = filePath.filename();

            // system("cls");
            std::cout << "Processing file: " << filename << "\n";

            DataExporter dataExporter(filePath);
            FileSize_uint fileCompressedSize = headerLoaderIterator.fileQueue.front().second;

            // 获取原始文件大小（未压缩的大小）用于控制解压输出
            FileSize_uint originalFileSize = headerLoaderIterator.fileQueue.front().first.getFileSize();
            FileSize_uint totalDecompressedBytes = 0; // 跟踪已经解压的总字节数

            std::cout << "[DEBUG] Original file size: " << originalFileSize << " bytes\n";
            std::cout << "[DEBUG] Compressed file size: " << fileCompressedSize << " bytes\n";

            // 处理文件的每个块：每个块都有独立的 Huffman 树
            while (totalDecompressedBytes < originalFileSize && fileCompressedSize > 0)
            {
                std::cout << "\n[DECOMPRESS] === Block start: decompressed " << totalDecompressedBytes << " / " << originalFileSize;
                std::cout << ", compressed remaining: " << fileCompressedSize << " bytes ===\n";

                // 在每次循环中重新创建 NumsReader 以确保正确读取
                NumsReader numReader(inFile);

                // 1. 读取分隔标志
                std::cout << "[DECOMPRESS] Reading tree SEPARATED_FLAG...\n";
                if (!(numReader.readBinaryNums<char>() == SEPARATED_FLAG))
                    throw std::runtime_error("decompressionLoop()-Error:Can't read SEPARATED_FLAG before tree block");

                // 2. 读取 Huffman 树块大小
                std::streampos treeSizePos = inFile.tellg();
                std::cout << "[DECOMPRESS] Reading tree block size from position: " << treeSizePos << "...\n";
                DirectoryOffsetSize_uint treeBlockSize = numReader.readBinaryNums<DirectoryOffsetSize_uint>();
                std::cout << "[DECOMPRESS] Tree block size: " << treeBlockSize << " bytes (includes 16B IV header)\n";

                // 检查是否还有足够的数据
                if (treeBlockSize > fileCompressedSize)
                {
                    throw std::runtime_error("decompressionLoop()-Error: treeBlockSize (" +
                                             std::to_string(treeBlockSize) + ") exceeds remaining fileCompressedSize (" +
                                             std::to_string(fileCompressedSize) + ")");
                }

                // 3. 读取并解密树数据
                std::cout << "[DECOMPRESS] Loading encrypted tree data (with IV)...\n";
                std::cout << "[DECOMPRESS] About to read " << treeBlockSize << " bytes for tree\n";

                DataBlock rawTreeData(treeBlockSize);
                inFile.read(reinterpret_cast<char *>(rawTreeData.data()), treeBlockSize);
                std::streamsize bytesRead = inFile.gcount();

                std::cout << "[DECOMPRESS] Successfully read " << bytesRead << " bytes\n";

                if (bytesRead != static_cast<std::streamsize>(treeBlockSize))
                {
                    throw std::runtime_error("decompressionLoop()-Error: Failed to read complete tree block. Expected " +
                                             std::to_string(treeBlockSize) + " bytes, got " + std::to_string(bytesRead));
                }

                std::cout << "[DECOMPRESS] Raw tree data size: " << rawTreeData.size() << " bytes\n";

                // 解密树数据(doAes会重新分配outputBuffer,不需要预先指定大小)
                std::cout << "[DECOMPRESS] Decrypting tree data...\n";
                DataBlock decryptedTreeData;
                aes.doAes(2, rawTreeData, decryptedTreeData);
                std::cout << "[DECOMPRESS] Decrypted tree data size: " << decryptedTreeData.size() << " bytes (IV removed)\n";

                // 打印解密后的数据前几个字节
                std::cout << "[DEBUG] First bytes of decrypted tree: ";
                for (size_t i = 0; i < std::min(size_t(30), decryptedTreeData.size()); ++i)
                {
                    if (decryptedTreeData[i] >= 32 && decryptedTreeData[i] <= 126)
                    {
                        std::cout << (char)decryptedTreeData[i];
                    }
                    else
                    {
                        std::cout << "[0x" << std::hex << (int)(unsigned char)decryptedTreeData[i] << std::dec << "]";
                    }
                }
                std::cout << "\n";

                // 恢复 Huffman 树（为这个块创建新树）
                std::cout << "[DEBUG] Spawning Huffman tree...\n";
                huffmanUnzip.spawn_tree(decryptedTreeData);

                // 检查树是否正确构建
                if (huffmanUnzip.getTreeRoot() == nullptr)
                {
                    throw std::runtime_error("decompressionLoop()-Error: Failed to spawn Huffman tree - tree root is NULL");
                }
                std::cout << "[DEBUG] Huffman tree spawned successfully\n";

                // 更新剩余大小: 减去树数据块本身的大小
                // 注意: FLAG 和 size 字段不计入 fileCompressedSize
                fileCompressedSize -= treeBlockSize;

                // 4. 读取分割标志(数据块前的标志)
                std::cout << "[DECOMPRESS] Reading data SEPARATED_FLAG...\n";
                if (!(numReader.readBinaryNums<char>() == SEPARATED_FLAG))
                    throw std::runtime_error("decompressionLoop()-Error:Can't read SEPARATED_FLAG before data block");

                // 5. 读取数据块大小
                std::streampos dataSizePos = inFile.tellg();
                std::cout << "[DECOMPRESS] Reading data block size from position: " << dataSizePos << "...\n";
                DirectoryOffsetSize_uint blockSize = numReader.readBinaryNums<DirectoryOffsetSize_uint>();
                std::cout << "[DECOMPRESS] Data block size: " << blockSize << " bytes (includes 16B IV header)\n";

                // 安全检查
                DirectoryOffsetSize_uint readSize = blockSize;
                if (readSize > fileCompressedSize)
                {
                    throw std::runtime_error("decompressionLoop()-Error: blockSize (" +
                                             std::to_string(readSize) + ") exceeds remaining fileCompressedSize (" +
                                             std::to_string(fileCompressedSize) + ")");
                }

                // 6. 读取加密的压缩数据
                std::cout << "[DECOMPRESS] Loading encrypted compressed data (with IV)...\n";
                std::cout << "[DECOMPRESS] About to read " << readSize << " bytes for data\n";

                DataBlock rawData(readSize);
                inFile.read(reinterpret_cast<char *>(rawData.data()), readSize);
                std::streamsize bytesRead2 = inFile.gcount();

                std::cout << "[DECOMPRESS] Successfully read " << bytesRead2 << " bytes\n";

                if (bytesRead2 != static_cast<std::streamsize>(readSize))
                {
                    throw std::runtime_error("decompressionLoop()-Error: Failed to read complete data block. Expected " +
                                             std::to_string(readSize) + " bytes, got " + std::to_string(bytesRead2));
                }

                std::cout << "[DECOMPRESS] Raw data size: " << rawData.size() << " bytes\n";

                // 解密数据(doAes会重新分配outputBuffer,不需要预先指定大小)
                std::cout << "[DECOMPRESS] Decrypting compressed data...\n";
                DataBlock decryptedData;
                aes.doAes(2, rawData, decryptedData);
                std::cout << "[DECOMPRESS] Decrypted data size: " << decryptedData.size() << " bytes (IV removed)\n";

                // 7. 使用该块对应的 Huffman 树进行解压
                std::cout << "[DECOMPRESS] Decoding with Huffman tree...\n";

                // 计算这个块最多应该解压多少字节
                // 每个块固定解压8192字节，只有最后一个块使用剩余字节数
                size_t remainingBytes = originalFileSize - totalDecompressedBytes;
                size_t maxBytesThisBlock = std::min(remainingBytes, (size_t)BUFFER_SIZE);
                std::cout << "[DECOMPRESS] Remaining bytes to decompress: " << remainingBytes;
                std::cout << ", max this block: " << maxBytesThisBlock << "\n";

                DataBlock decompressedData;
                huffmanUnzip.decode(decryptedData, decompressedData, BitHandler(), maxBytesThisBlock);
                std::cout << "[DECOMPRESS] Decompressed data size: " << decompressedData.size() << " bytes\n";

                // 调试: 打印解压数据的前几个字节
                std::cout << "[DEBUG] First bytes of decompressed data: ";
                for (size_t i = 0; i < std::min(size_t(30), decompressedData.size()); ++i)
                {
                    if (decompressedData[i] >= 32 && decompressedData[i] <= 126)
                    {
                        std::cout << (char)decompressedData[i];
                    }
                    else
                    {
                        std::cout << "[0x" << std::hex << (int)(unsigned char)decompressedData[i] << std::dec << "]";
                    }
                }
                std::cout << "\n";

                // 更新已解压的总字节数
                totalDecompressedBytes += decompressedData.size();
                std::cout << "[DECOMPRESS] Total decompressed: " << totalDecompressedBytes << " / " << originalFileSize << " bytes\n";

                // 8. 写入解压后的数据
                std::cout << "[DECOMPRESS] Writing decompressed data to file...\n";
                dataExporter.exportDataToFile_Decompression(decompressedData);

                // 更新剩余大小: 减去压缩数据块本身的大小
                // 注意: FLAG 和 size 字段不计入 fileCompressedSize
                fileCompressedSize -= readSize;
                std::cout << "[DECOMPRESS] Block complete. Remaining compressed size: " << fileCompressedSize << " bytes\n\n";

                // 文件流已经自动前进，不需要手动seekg
                // dataOffset会在下一个文件开始时更新
            }
            std::cout << "\n========================================\n";
            std::cout << "File decompression complete: " << filename << "\n";
            std::cout << "Final size: " << totalDecompressedBytes << " / " << originalFileSize << " bytes\n";
            std::cout << "========================================\n\n";

            // 更新dataOffset为下一个文件的起始位置
            DirectoryOffsetSize_uint oldOffset = dataOffset;
            dataOffset = inFile.tellg();
            std::cout << "[DECOMPRESS] File done. Updated dataOffset from " << oldOffset << " to " << dataOffset << "\n";

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
