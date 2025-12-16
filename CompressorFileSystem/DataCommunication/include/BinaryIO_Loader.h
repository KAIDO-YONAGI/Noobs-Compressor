#pragma once

#include "FileLibrary.h"
#include "ToolClasses.h"
#include "Directory_FileDetails.h"
#include "Directory_FileParser.h"
#include "My_Aes.h"
#include <queue>
class BinaryIO_Loader
{
private:
    bool blockIsDone = false;
    bool allDone = false;   // ����Ƿ��������Ŀ¼��ȡ
    bool FirstReady = true; // ��ǵ�ǰ�Ƿ���Ŀ¼�������е�һ���?���?

    FileCount_uint countOfKidDirectory = 0;  // ��ǰ�����л��˳�ʱĿ¼����Ŀ¼���ļ�����
    DirectoryOffsetSize_uint offset = 0;     // ��ǰʣ���ֽ���
    DirectoryOffsetSize_uint tempOffset = 0; // ��ǰ������Ĵ�С���?��ƣ�?

    fs::path loadPath;
    fs::path parentPath;
    std::ifstream inFile;
    std::fstream fstreamForRefill;
    std::vector<std::string> filePathToScan; // ����ʱ��ʼ��������ֻʹ��һ��

    Transfer transfer;
    Header header;                         // ˽�л��洢��ǰ�ļ�ͷ��Ϣ
    std::unique_ptr<Directory_FileParser>parserForLoader; // ˽�л�������ʵ���������ظ�����������
    DataBlock buffer =
        DataBlock(BUFFER_SIZE + 1024); // ˽��buffer,Ԥ��1024�ֽڷ�ֹ���?

    void requestDone();

    void allLoopDone();

    void loadBySepratedFlag(NumsReader &numsReader, FileCount_uint &countOfKidDirectory, Aes &aes);

public:
    void headerLoaderIterator(Aes &aes); // ���߼�����

    // ѹ��ʱ����
    Directory_FileQueue fileQueue;                            // �ļ�����
    Directory_FileQueue directoryQueue;                       // Ŀ¼����
    std::vector<std::array<DirectoryOffsetSize_uint, 2>> pos; // Ŀ¼���ݿ�λ������ 1 Ϊ���?2Ϊ��С

    // ��ѹʱ����
    std::queue<fs::path> directoryQueue_ready; // Ŀ¼�ָ��������У��ļ���ԭ��Ҫ��Ŀ¼�ָ������?

    BinaryIO_Loader() {};
    BinaryIO_Loader(const std::string inPath, std::vector<std::string> filePathToScan, fs::path parentPath)
    {
        this->loadPath = transfer.transPath(inPath);
        this->inFile.open(loadPath, std::ios::binary);
        this->fstreamForRefill.open(loadPath, std::ios::binary | std::ios::in | std::ios::out);
        if (!this->inFile)
            throw std::runtime_error("BinaryIO_Loader()-Error:Failed to open inFile" + inPath);
        if (!this->fstreamForRefill)
            throw std::runtime_error("BinaryIO_Loader()-Error:Failed to open fstreamForRefill" + inPath);

        this->filePathToScan = filePathToScan;
        this->parserForLoader = std::make_unique<Directory_FileParser>
            (buffer, directoryQueue, fileQueue, header, offset, tempOffset, this->filePathToScan);
        this->parentPath = parentPath;
    }

    ~BinaryIO_Loader() { allLoopDone(); }

    DirectoryOffsetSize_uint getDirectoryOffset() { return header.directoryOffset; }

    bool allLoopIsDone() { return allDone; }
    bool loaderRequestIsDone() { return blockIsDone; }
    std::ifstream &getInFile() { return inFile; }
    void restartLoader();
    void encryptHeaderBlock(Aes &aes);
};
