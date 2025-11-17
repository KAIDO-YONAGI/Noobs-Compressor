// HeaderReader.h
#ifndef HEADERREADER
#define HEADERREADER

#include "../include/FileLibrary.h"
#include "../include/FileQueue.h"
#include "../include/FileDetails.h"
#include "../include/Transfer.h"
class FilePath; //类的前向声明
class BinaryIO_Reader;

class HeaderReader
{
public:
    HeaderReader() = default;
    void writeRoot(FilePath &File);
    void scanFlow(FilePath &File);
    void appendMagicStatic(fs::path &outputFilePath);
    void headerReader(std::string &path);
    FileCount_Int countFilesInDirectory(fs::path &filePathToScan);
};

class FilePath
{
private:
    fs::path outPutFilePath;
    fs::path filePathToScan;

public:
    FilePath(const FilePath &) = delete;
    FilePath(fs::path &outPutFilePath, fs::path &filePathToScan)
        : outPutFilePath(outPutFilePath), filePathToScan(filePathToScan) {}

    void setFilePathToScan(fs::path &filePathToScan)
    {
        this->filePathToScan = filePathToScan;
    }

    fs::path &getOutPutFilePath() { return outPutFilePath; }
    fs::path &getFilePathToScan() { return filePathToScan; }
};

class BinaryIO_Reader
{
public:
    BinaryIO_Reader() = default;
    FileSize_Int getFileSize(fs::path &filePathToScan);
    void scanner(FilePath &File, QueueInterface &queue);
    void writeBinaryStandard(std::ofstream &outfile, FileDetails &details, QueueInterface &queue);
    void writeHeaderStandard(std::ofstream &outfile, FileDetails &details, FileCount_Int count);
    void writeFileStandard(std::ofstream &outfile, FileDetails &details);
};

template <typename T>
void write_binary_le(std::ofstream &file, T value)
{
    file.write(reinterpret_cast<char *>(&value), sizeof(T)); //不做类型检查，直接进行类型转换
}

#endif
