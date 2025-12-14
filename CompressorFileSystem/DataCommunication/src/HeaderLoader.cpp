#include "../include/HeaderLoader.h"
#include "Heffman.h"
void HeaderLoader_Compression ::headerLoader(const std::string compressionFilePath, const std::vector<std::string> &filePathToScan, Aes &aes)
{
    // 初始化迭代器
    BinaryIO_Loader headerLoaderIterator(compressionFilePath, filePathToScan);
    Heffman huffmanZip(1);
    DataBlock encryptedBlock;
    FileSize_uint totalBlocks = 1, count = 0;
    headerLoaderIterator.headerLoaderIterator(aes); // 执行第一次操作，把根目录载入
    if (!headerLoaderIterator.fileQueue.empty())    // 单个文件特殊处理
    {
        Directory_FileDetails loadFile = headerLoaderIterator.fileQueue.front().first;
        loadPath = loadFile.getFullPath();
        dataLoader = new DataLoader(loadPath);
        totalBlocks = (loadFile.getFileSize() + BUFFER_SIZE - 1) / BUFFER_SIZE;
    }

    DataExporter dataExporter(transfer.transPath(compressionFilePath));

    fs::path filename = loadPath.filename();

    bool isGenerated = false;

    while (!headerLoaderIterator.fileQueue.empty())
    {
        // if (!isGenerated)
        // {
        //     std::cout<<"genTree"<<"\n";
        //     while (!dataLoader->isDone())
        //     {
        //         dataLoader->dataLoader();
        //         huffmanZip.statistic_freq(0, dataLoader->getBlock());
        //     }
        //     huffmanZip.merge_ttabs();
        //     huffmanZip.gen_hefftree();
        //     huffmanZip.save_code_inTab();
        //     DataBlock huffTree;
        //     DataBlock huffTree_outPut;
        //     huffmanZip.tree_to_plat_uchar(huffTree);
        //     aes.doAes(1, huffTree, huffTree_outPut);
        //     dataExporter.exportDataToFile_Encryption(huffTree_outPut);
        //     Directory_FileDetails loadFile = headerLoaderIterator.fileQueue.front().first;
        //     dataLoader->reset(loadFile.getFullPath()); // 生成编码表后，调用reset（原目录）复读
        //     isGenerated = true;
        // }

        dataLoader->dataLoader();

        if (!dataLoader->isDone()) // 避免读到空数据块
        {
            if (filename == "deflater.js")
                int a = 1;

            system("cls");
            std::cout << "Processing file: " << filename << "\n"
                      << std::fixed << std::setw(6) << std::setprecision(2)
                      << (100.0 * ++count) / totalBlocks
                      << "% \n";
            encryptedBlock.resize(BUFFER_SIZE + sizeof(IvSize_uint));

            // DataBlock data_In = dataLoader->getBlock(); // 调用压缩
            // DataBlock compressedData;
            // huffmanZip.encode(data_In, compressedData);

            // aes.doAes(1, compressedData, encryptedBlock);
            aes.doAes(1, dataLoader->getBlock(), encryptedBlock);

            dataExporter.exportDataToFile_Encryption(encryptedBlock); // 读取的数据传输给exporter
            encryptedBlock.clear();
        }
        else if (dataLoader->isDone() && !headerLoaderIterator.fileQueue.empty())
        {
            FileNameSize_uint offsetToFill = headerLoaderIterator.fileQueue.front().second;
            dataExporter.thisFileIsDone(offsetToFill); // 可在此前插入一个编码表写入再调用done
            std::cout << "--------Done!--------" << "\n";

            headerLoaderIterator.fileQueue.pop();
            if (!headerLoaderIterator.fileQueue.empty())
            { // 更新下一个文件路径
                Directory_FileDetails newLoadFile = headerLoaderIterator.fileQueue.front().first;
                dataLoader->reset(newLoadFile.getFullPath());
                filename = newLoadFile.getFullPath().filename();
                totalBlocks = (newLoadFile.getFileSize() + BUFFER_SIZE - 1) / BUFFER_SIZE;
                count = 0;
                isGenerated = false;
            }
        }

        if (headerLoaderIterator.fileQueue.empty() && !headerLoaderIterator.allLoopIsDone()) // 队列空但整体未完成，请求下一轮读取对队列进行填充
        {
            headerLoaderIterator.restartLoader();
            headerLoaderIterator.headerLoaderIterator(aes);
        }
    }
    // headerLoaderIterator.encryptHeaderBlock(aes); // 加密目录块并且回填

    delete dataLoader;
}