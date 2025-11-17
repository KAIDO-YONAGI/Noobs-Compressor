// HeaderReader.h
#ifndef HEADERREADER
#define HEADERREADER

#include "../include/FileLibrary.h"
#include "../include/FileQueue.h"

class FilePath; //类的前向声明
class Locator;
class FileDetails;
class QueueInterface;
class BinaryIO;

class HeaderReader
{

public:
    HeaderReader() = default;
    void writeRoot(FilePath &File);
    void scanFlow(FilePath &File);
    void appendMagicStatic(fs::path &outputFilePath);
    void headerReader(std::string& path);
};

class FileDetails
{
private:
    std::string name;
    FileNameSize_Int sizeOfName;
    FileSize_Int fileSize;
    bool isFile;
    fs::path fullPath;

public:
    FileDetails() = default;
    std::string &getName() { return name; }
    fs::path &getFullPath() { return fullPath; }
    FileNameSize_Int getSizeOfName() { return sizeOfName; }
    FileSize_Int getFileSize() { return fileSize; }
    bool getIsFile() { return isFile; }

    FileDetails(std::string name, FileNameSize_Int sizeOfName, FileSize_Int fileSize, bool isFile, fs::path &fullPath)
        : name(std::move(name)), sizeOfName(sizeOfName), fileSize(fileSize), isFile(isFile), fullPath(fullPath) {}
};
class FilePath
{
private:
    fs::path outPutFilePath;
    fs::path filePathToScan;

public:
    FilePath(const FilePath&)=delete;
    FilePath(fs::path &outPutFilePath, fs::path &filePathToScan)
        : outPutFilePath(outPutFilePath), filePathToScan(filePathToScan) {}

    void setFilePathToScan(fs::path &filePathToScan)
    {
        this->filePathToScan = filePathToScan;
    }

    fs::path &getOutPutFilePath() { return outPutFilePath; }
    fs::path &getFilePathToScan() { return filePathToScan; }
};


class BinaryIO
{
public:
    BinaryIO() = default;
    FileSize_Int getFileSize(fs::path &filePathToScan);
    void scanner(FilePath &File, QueueInterface &queue);
    void writeBinaryStandard(std::ofstream &outfile, FileDetails &details, QueueInterface &queue);
    void writeHeaderStandard(std::ofstream &outfile, FileDetails &details, FileCount_Int count);
    void writeFileStandard(std::ofstream &outfile, FileDetails &details);
};

bool fileIsExist(fs::path &outPutFilePath);
FileCount_Int countFilesInDirectory(fs::path &filePathToScan);

template <typename T>
void write_binary_le(std::ofstream &file, T value)
{
    file.write(reinterpret_cast<char *>(&value), sizeof(T)); //不做类型检查，直接进行类型转换
}


#endif
