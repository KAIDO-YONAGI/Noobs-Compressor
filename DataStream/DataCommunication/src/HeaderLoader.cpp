#include "../include/HeaderLoader.h"

void BinaryIO_Loader::headerLoader(std::vector<std::string> &filePathToScan)
{
    try
    {

        // 读取Header
        if (!inFile.read(reinterpret_cast<char *>(buffer.data()), HeaderSize))
        {
            throw std::runtime_error("Failed to read header");
        }
        // 解释Header
        const BinaryIO_Loader::Header *header =
            reinterpret_cast<const BinaryIO_Loader::Header *>(buffer.data());
        // 验证魔数
        if (header->magicNum_1 != MagicNum ||
            header->magicNum_2 != MagicNum)
        {
            throw std::runtime_error("Invalid file format");
        }
        NumsReader numsReader(inFile);
        DirectoryOffsetSize_uint offset = header->directoryOffset - HeaderSize;
        FileCount_uint countOfKidDirectory = 0;

        while (offset / BufferSize > 0 || offset % BufferSize > 0)
        {

            buffer.clear();
            loadBySepratedFlag(numsReader, offset, filePathToScan, countOfKidDirectory);
            if (1)
                continue;
            // 最后把目录数据块覆写回原位置（已经回填偏移量。如果有加密，则加密后再填，并且要在分割处写入iv头）
        }

        // 读末尾魔数并且检验
        SizeOfMagicNum_uint magicNum = numsReader.readBinaryNums<SizeOfMagicNum_uint>();
        if (magicNum != MagicNum)
            throw std::runtime_error("Invalid MagicNum");
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        // 清理资源或重新抛出异常
        throw;
    }
}
void BinaryIO_Loader::loadBySepratedFlag(NumsReader &numsReader, DirectoryOffsetSize_uint &offset, std::vector<std::string> &filePathToScan, FileCount_uint &countOfKidDirectory)
{
    if (offset == 0)
        return;
    unsigned char flag = numsReader.readBinaryNums<unsigned char>();

    if (flag == '2')
    {

        // 读取块偏移量
        DirectoryOffsetSize_uint tempOffset = numsReader.readBinaryNums<DirectoryOffsetSize_uint>();
        // 读取iv头
        IvSize_uint ivNum = numsReader.readBinaryNums<IvSize_uint>();

        offset -= (SeparatedStandardSize + tempOffset); // 偏移量检测，同样用于检测退出（注意三目运算符的优先级）

        // 读取数据到vector后在内存中操作，对最后一个未达到写入分割标准大小的块引入特殊处理
        DirectoryOffsetSize_uint readSize = (tempOffset == 0 ? (offset - sizeof(SizeOfMagicNum_uint)) : tempOffset);

        // 按偏移量读取数据块
        buffer.resize(readSize); // clear后resize确保空间可写入，不改变capacity
        if (!inFile.read(reinterpret_cast<char *>(buffer.data()), readSize) && tempOffset != 0)
        {
            throw std::runtime_error("Failed to read buffer");
        }

        DirectoryOffsetSize_uint bufferPtr = 0;

        while (readSize>bufferPtr) //(readSize<BufferSize)表示末尾块
        {
            while (countOfKidDirectory > 0 || bufferPtr == 0)
            {
                if (readSize <= bufferPtr)
                    return;
                parser(tempOffset, bufferPtr, filePathToScan, countOfKidDirectory);

                // int a = 0;
                // if (countOfDirec == 1166)
                //     a++;
            }

            if (!queue.empty())
            {

                // countOfDirec++; // 临时全局变量
                std::cout << queue.front().first.getName() << " " <<
                //  countOfDirec <<
                  " " << queue.size() << " ";

                queue.pop();
                if (!queue.empty())
                    countOfKidDirectory = queue.front().second;
            }
            else break;
        }
        if (tempOffset == 0)
            offset -= readSize + sizeof(MagicNum); // tempOffset为零，说明到末尾，减去对应偏移量，包含魔数大小是为了结束循环
        // std::cout << "\n"
        //           << readSize << " " << offset << "\n";
    }
    else
        throw std::runtime_error("loadBySepratedFlag()-Error:Failed to read separatedFlag");
}
void BinaryIO_Loader::parser(DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &bufferPtr, std::vector<std::string> &filePathToScan, FileCount_uint &countOfKidDirectory)
{
    if (tempOffset <= bufferPtr && tempOffset != 0)
        return;

    unsigned char D_F_flag = numsParser<unsigned char>(bufferPtr);
    switch (D_F_flag)
    {
    case '1':
    {
        fileParser(bufferPtr);
        countOfKidDirectory--;
        break;
    }
    case '0':
    {
        directoryParser(bufferPtr);
        countOfKidDirectory--;
        break;
    }
    case '3': // 逻辑根本身不入队，入队接下来的几个根目录，并且处理文件
    {
        rootParser(bufferPtr, filePathToScan);
        countOfKidDirectory = queue.front().second; // 启动递推
        break;
    }

    default:
    {

        throw std::runtime_error("parser()-Error:Failed to read flag");
        break;
    }
    }
}
void BinaryIO_Loader::fileParser(DirectoryOffsetSize_uint &bufferPtr)
{
    // 解析文件名偏移量
    // 解析文件名，后续拼接为绝对路径之后交给数据读取类读取数据块
    FileNameSize_uint fileNameSize = 0;
    std::string fileName;
    fileName_fileSizeParser(fileNameSize, fileName, bufferPtr);

    // 解析文件原大小
    FileSize_uint originSize = numsParser<FileSize_uint>(bufferPtr);

    // 记录等会需要回填的位置
    FileSize_uint compressedSizeOffset = bufferPtr;
    bufferPtr += sizeof(FileSize_uint);
}
void BinaryIO_Loader::directoryParser(DirectoryOffsetSize_uint &bufferPtr)
{
    // 解析目录名偏移量
    // 解析目录名，后续拼接为绝对路径之后入队
    FileNameSize_uint directoryNameSize = 0;
    std::string directoryName;
    fileName_fileSizeParser(directoryNameSize, directoryName, bufferPtr);

    // 解析下级文件数量
    FileCount_uint count = numsParser<FileCount_uint>(bufferPtr);

    // 需要入队（BFS） 
    fs::path lastPath;
    fs::path newPath;
    if (!queue.empty())
    {
        lastPath = queue.front().first.getFullPath();
        newPath = lastPath / directoryName;
    }
    if (fs::exists(newPath))
    {
        FileDetails directoryDetails(directoryName, directoryNameSize, 0, false, newPath);
        queue.push({directoryDetails, count});
    }
    else
        throw std::runtime_error("directoryParser()-Error:No such path");
}

void BinaryIO_Loader::rootParser(DirectoryOffsetSize_uint &bufferPtr, std::vector<std::string> &filePathToScan)
{

    FileNameSize_uint directoryNameSize = 0;
    std::string directoryName;
    fileName_fileSizeParser(directoryNameSize, directoryName, bufferPtr);
    // 解析下级文件数量
    FileCount_uint count = numsParser<FileCount_uint>(bufferPtr);

    if (count != filePathToScan.size()) // 检验数量
        throw std::runtime_error("rootParser()-Error:Failed to match RootDirectory nums");

    for (std::string &path : filePathToScan)
    {
        fs::path fullPath = transfer.transPath(path);
        if (fs::is_regular_file(fullPath))
        {
            bufferPtr += sizeof(SizeOfFlag_uint); // 步过文件标
            fileParser(bufferPtr);                // 遇到文件直接处理，调用fileParser}
        }
        else if (fs::is_directory(fullPath))
        {
            bufferPtr += sizeof(SizeOfFlag_uint); // 步过文件标
            FileNameSize_uint directoryNameSize = 0;
            std::string directoryName;
            fileName_fileSizeParser(directoryNameSize, directoryName, bufferPtr);
            // 解析下级文件数量
            FileCount_uint count = numsParser<FileCount_uint>(bufferPtr);

            FileDetails directoryDetails(directoryName, directoryNameSize, 0, false, fullPath);
            queue.push({directoryDetails, count});
        }
    }
}
void dataLoaderForHuffmanCompression(fs::path inParh)
{
    std::ifstream inFile(inParh, std::ios::binary);
    if (!inFile)
        throw std::runtime_error("dataLoaderForHuffmanCompression()-Error:Failed to open inFile");
}