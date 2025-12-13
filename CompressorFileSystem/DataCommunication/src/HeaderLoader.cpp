#include "../include/HeaderLoader.h"

void HeaderLoader_Compression ::headerLoader(const std::string compressionFilePath, const std::vector<std::string> &filePathToScan, Aes &aes)
{
    // 初始化加载器
    BinaryIO_Loader headerLoaderIterator(compressionFilePath, filePathToScan);
    DataBlock encryptedBlock;
    FileSize_uint totalBlocks = 1, count = 0;
    headerLoaderIterator.headerLoaderIterator();         // 执行第一次操作，把根目录载入
    if (!headerLoaderIterator.fileQueue.empty()) // 单个文件特殊处理
    {
        Directory_FileDetails &loadFile = headerLoaderIterator.fileQueue.front().first;
        loadPath = loadFile.getFullPath();
        dataLoader = new DataLoader(loadPath);
        totalBlocks = (loadFile.getFileSize() + BUFFER_SIZE - 1) / BUFFER_SIZE;
    }

    DataExporter dataExporter(transfer.transPath(compressionFilePath));

    fs::path filename = loadPath.filename();
    while (!headerLoaderIterator.fileQueue.empty())
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
        else if (dataLoader->isDone() && !headerLoaderIterator.fileQueue.empty())
        {
            FileNameSize_uint offsetToFill = headerLoaderIterator.fileQueue.front().second;
            dataExporter.thisFileIsDone(offsetToFill); // 可在此前插入一个编码表写入再调用done
            std::cout << "--------Done!--------" << "\n";

            headerLoaderIterator.fileQueue.pop();
            if (!headerLoaderIterator.fileQueue.empty())
            { // 更新下一个文件路径，生成编码表时可以调用reset（原目录）读两轮
                Directory_FileDetails &loadFile = headerLoaderIterator.fileQueue.front().first;
                dataLoader->reset(loadFile.getFullPath());
                filename = loadFile.getFullPath().filename();
                totalBlocks = (loadFile.getFileSize() + BUFFER_SIZE - 1) / BUFFER_SIZE;
                count = 0;
            }
        }

        if (headerLoaderIterator.fileQueue.empty() && !headerLoaderIterator.allLoopIsDone()) // 队列空但整体未完成，请求下一轮读取对队列进行填充
        {
            headerLoaderIterator.restartLoader();
            headerLoaderIterator.headerLoaderIterator();
        }
    }
}