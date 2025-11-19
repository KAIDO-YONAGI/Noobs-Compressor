// FileDetails.h
#ifndef FILEDETAILS
#define FILEDETAILS

#include "../include/FileLibrary.h"

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
    FilePath(const FilePath &) = delete;
    FilePath() {}
    void setOutPutFilePath(fs::path &outPutFilePath)
    {
        this->outPutFilePath = outPutFilePath;
    }
    void setFilePathToScan(fs::path &filePathToScan)
    {
        this->filePathToScan = filePathToScan;
    }

    fs::path &getOutPutFilePath() { return outPutFilePath; }
    fs::path &getFilePathToScan() { return filePathToScan; }
};
#endif