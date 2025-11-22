// Directory_FileWriter.h
#ifndef FILEWRITER_H
#define FILEWRITER_H

#include "../include/FileLibrary.h"
class HeaderWriter_v0
{

    void header_v0(std::ofstream &outFile, fs::path &fullOutPath);
    void directory_v0(std::ofstream &outFile, const std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot);
public:
    HeaderWriter_v0() = default;
    void headerWriter(std::vector<std::string> &filePathToScan, std::string &outPutFilePath, const std::string &logicalRoot);
    
};

#endif