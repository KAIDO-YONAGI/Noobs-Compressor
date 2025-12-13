#include "../include/BinaryIO.h"
void BinaryIO_Loader::headerLoaderIterator()
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
        requesetDone();      // 单次请求完成
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

void BinaryIO_Writter::binaryIO_Reader(FilePath &file, Directory_FIleQueueInterface &directoryQueue, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset)
{

    try
    {

        for (const fs::directory_entry &entry : fs::directory_iterator(file.getFilePathToScan()))
        {
            bool File_Direc = true;
            std::string name;
            fs::path fullPath;
            FileNameSize_uint sizeOfName;
            FileSize_uint fileSize;
            if (entry.is_regular_file())
            {
                File_Direc = true;
                fileSize = entry.file_size();
            }

            else if (entry.is_directory())
            {
                File_Direc = false;
                fileSize = 0;
            }

            else if (entry.is_symlink())
            {
                File_Direc = false;
                fileSize = 1; // 大小为一，仅表示是符号链接
            }
            else
                continue; // 禁用三个基本文件类型之外的文件类型
            name = entry.path().filename().string();
            fullPath = entry.path();
            sizeOfName = name.size();
            Directory_FileDetails details(
                name,
                sizeOfName,
                fileSize,
                File_Direc,
                fullPath); // 创建details

            writeStorageStandard(details, directoryQueue, tempOffset, offset);
        }
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "binaryIO_Reader()-Error: " << e.what() << "\n";
    }
}
// 识别存储标准并且分发到各个写入函数
void BinaryIO_Writter::writeStorageStandard(Directory_FileDetails &details, Directory_FIleQueueInterface &directoryQueue, DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint &offset)
{

    if (details.getIsFile()) // 文件对应的处理
    {
        writeFileStandard(details, tempOffset);
    }
    else if ((!details.getIsFile()) && (details.getFileSize() == 0)) // 目录对应的处理
    {
        FileCount_uint countOfThisDirectory = countFilesInDirectory(details.getFullPath());

        directoryQueue.Directory_FileQueue.push({details, countOfThisDirectory}); // 如果是目录则存入其details与其子文件数目的std::pair 到队列中备用
        writeDirectoryStandard(details, countOfThisDirectory, tempOffset);
    }
    else if ((!details.getIsFile()) && (details.getFileSize() == 1))
    {
        writeSymbolLinkStandard(details, tempOffset);
    }
    if (tempOffset >= BUFFER_SIZE) // 达到缓冲大小后写入分割标准
    {

        writeSeparatedStandard(tempOffset, offset);
        offset += tempOffset;
        // 写入完毕后将当前相对位置（用多个存储标准偏移量累加维护的tempOffset）加到文件的总偏移量offset上，以维护整个偏移逻辑
        tempOffset = 0; // 相对位置归零

        // 预留下一次回填的位置
        writeBlankSeparatedStandard();
        offset += SEPARATED_STANDARD_SIZE; // 更新offset，保证回填正确。不更新tempOffset，为的是将分割标准的大小排除在外，便于拿到偏移量能不经变换直接操作对应位置的数据
    }
}
// 目录标准写入函数
void BinaryIO_Writter::writeDirectoryStandard(Directory_FileDetails &details, FileCount_uint count, DirectoryOffsetSize_uint &tempOffset)
{
    FileNameSize_uint sizeOfName = details.getSizeOfName();

    tempOffset += DIRECTORY_STANDARD_SIZE_BASIC + sizeOfName;

    outFile.write(HEADER_FLAG, FLAG_SIZE);
    numWriter.writeBinaryNums(sizeOfName, outFile);
    outFile.write(details.getName().c_str(), sizeOfName);
    numWriter.writeBinaryNums(count, outFile); // 写入文件数目
}
// 文件标准写入函数
void BinaryIO_Writter::writeFileStandard(Directory_FileDetails &details, DirectoryOffsetSize_uint &tempOffset)
{
    FileNameSize_uint sizeOfName = details.getSizeOfName();

    tempOffset += FILE_STANDARD_SIZE_BASIC + sizeOfName;

    outFile.write(FILE_FLAG, FLAG_SIZE);                       // 先写文件标
    numWriter.writeBinaryNums(sizeOfName, outFile);            // 写入文件名偏移量
    outFile.write(details.getName().c_str(), sizeOfName);      // 写入文件名
    numWriter.writeBinaryNums(details.getFileSize(), outFile); // 写入文件大小
    numWriter.writeBinaryNums(FileSize_uint(0), outFile);      // 预留大小
}
// 分割标准写入函数（回填）
void BinaryIO_Writter::writeSeparatedStandard(DirectoryOffsetSize_uint &tempOffset, DirectoryOffsetSize_uint offset)
{
    locator.offsetLocator(outFile, offset + FLAG_SIZE);
    numWriter.writeBinaryNums(tempOffset, outFile);
    outFile.seekp(0, std::ios::end);
}
// 空分割标准写入函数
void BinaryIO_Writter::writeBlankSeparatedStandard()
{
    outFile.write(SEPARATED_FLAG, FLAG_SIZE);
    numWriter.writeBinaryNums(DirectoryOffsetSize_uint(0), outFile);
    numWriter.writeBinaryNums(IvSize_uint(0), outFile);
}
// 由于加密模式iv包含在数据区内，直接写入不含iv部分的空分割标准
void BinaryIO_Writter::writeBlankSeparatedStandardForEncryption(std::fstream &File)
{
    File.write(SEPARATED_FLAG, FLAG_SIZE);
    numWriter.writeBinaryNums(DirectoryOffsetSize_uint(0), File);
}
// 符号链接标准写入函数
void BinaryIO_Writter::writeSymbolLinkStandard(Directory_FileDetails &details, DirectoryOffsetSize_uint &tempOffset)
{
    FileNameSize_uint sizeOfName = details.getSizeOfName();
    FileNameSize_uint sizeOfPath = details.getFullPath().string().size();

    tempOffset += SYMBOL_LINK_STANDARD_SIZE_BASIC + sizeOfName + sizeOfPath;

    outFile.write(SYMBOL_LINK_FLAG, FLAG_SIZE);

    numWriter.writeBinaryNums(sizeOfName, outFile);
    numWriter.writeBinaryNums(sizeOfPath, outFile);

    outFile.write(details.getName().c_str(), sizeOfName);
    outFile.write(details.getFullPath().string().c_str(), sizeOfPath);
}
void BinaryIO_Writter::writeLogicalRoot(const std::string &logicalRoot, const FileCount_uint count, DirectoryOffsetSize_uint &tempOffset)
{
    FileNameSize_uint sizeOfName = logicalRoot.size();
    tempOffset += DIRECTORY_STANDARD_SIZE_BASIC + sizeOfName;

    outFile.write(LOGICAL_ROOT_FLAG, FLAG_SIZE);
    numWriter.writeBinaryNums(sizeOfName, outFile);
    outFile.write(logicalRoot.c_str(), sizeOfName);
    numWriter.writeBinaryNums(count, outFile); // 写文件数
}
void BinaryIO_Writter::writeRoot(FilePath &file, const std::vector<std::string> &filePathToScan, DirectoryOffsetSize_uint &tempOffset)
{
    FileCount_uint length = filePathToScan.size();
    for (FileCount_uint i = 0; i < length; i++)
    {

        fs::path sPath = transfer.transPath(filePathToScan[i]);

        if (fs::exists(sPath))
        {
            file.setFilePathToScan(sPath);
        }
        else
        {
            throw("directory_fileProcessor()-Error:file Not Exist: " + sPath.string() + "\n");
        }

        fs::path rootPath = file.getFilePathToScan(); // 获取根目录

        // 先写入根目录(或文件)自身（手动构造）

        std::string rootName = rootPath.filename().string();
        FileNameSize_uint rootNameSize = rootName.size();
        bool File_Direc = fs::is_regular_file(rootPath);
        FileSize_uint fileSize = File_Direc ? fs::file_size(rootPath) : 0;

        Directory_FileDetails rootDetails(
            rootName,     // 目录名 (如 "Folder")
            rootNameSize, // 名称长度
            fileSize,     // 文件大小(如果是文件)
            File_Direc,   // 是否为常规文件
            rootPath      // 完整路径
        );
        const fs::directory_entry entry(rootPath);
        if (entry.is_regular_file())
        {
            writeFileStandard(rootDetails, tempOffset);
        }
        else if (entry.is_directory())
        {
            FileCount_uint count = countFilesInDirectory(rootPath);
            writeDirectoryStandard(rootDetails, count, tempOffset);
        }
        else if (entry.is_symlink())
        {
            rootDetails.setFileSize(1);
            writeSymbolLinkStandard(rootDetails, tempOffset);
        }
    }
}
FileCount_uint BinaryIO_Writter::countFilesInDirectory(const fs::path &filePathToScan)
{
    try
    {
        return std::distance(fs::directory_iterator(filePathToScan), fs::directory_iterator{});
    }
    catch (fs::filesystem_error &e)
    {
        throw("countFilesInDirectory()-Error: " + std::string(e.what()) + "\n");
    }
}
FileSize_uint BinaryIO_Writter::getFileSize(const fs::path &filePathToScan)
{
    try
    {
        outFile.flush();

        return fs::file_size(filePathToScan);
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "getFileSize()-Error: " << e.what() << "\n";
        return 0;
    }
}
