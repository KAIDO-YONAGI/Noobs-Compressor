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
    const std::string &getName() { return name; }
    const fs::path &getFullPath() { return fullPath; }
    const FileNameSize_Int getSizeOfName() { return sizeOfName; }
    const FileSize_Int getFileSize() { return fileSize; }
    const bool getIsFile() { return isFile; }

    FileDetails(std::string name, FileNameSize_Int sizeOfName, FileSize_Int fileSize, bool isFile, fs::path &fullPath)
        : name(std::move(name)), sizeOfName(sizeOfName), fileSize(fileSize), isFile(isFile), fullPath(fullPath) {}
};

class FilePath
{
private:
    fs::path outPutFilePath;
    fs::path filePathToScan;

public:
    FilePath() {}
    void setOutPutFilePath(const fs::path &outPutFilePath)
    {
        this->outPutFilePath = outPutFilePath;
    }
    void setFilePathToScan(const fs::path &filePathToScan)
    {
        this->filePathToScan = filePathToScan;
    }

    const fs::path &getOutPutFilePath() { return outPutFilePath; }
    const fs::path &getFilePathToScan() { return filePathToScan; }
};
#endif