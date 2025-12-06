// FileDetails.h
#pragma once

#include "../include/FileLibrary.h"

class FileDetails
{
private:
    std::string name;
    FileNameSize_uint sizeOfName;
    FileSize_uint fileSize;
    bool isFile;
    fs::path fullPath;

public:
    FileDetails(std::string name, FileNameSize_uint sizeOfName, FileSize_uint fileSize, bool isFile, fs::path &fullPath)
        : name(std::move(name)), sizeOfName(sizeOfName), fileSize(fileSize), isFile(isFile), fullPath(fullPath) {}
    const std::string &getName() { return name; }
    const fs::path &getFullPath() { return fullPath; }
    const FileNameSize_uint getSizeOfName() { return sizeOfName; }
    const FileSize_uint getFileSize() { return fileSize; }
    const bool getIsFile() { return isFile; }
    void setFileSize(FileSize_uint fileSize) { this->fileSize = fileSize; }
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
