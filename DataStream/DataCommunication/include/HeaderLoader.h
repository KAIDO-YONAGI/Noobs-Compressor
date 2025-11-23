// HeaderLoader.h
#ifndef HEADERLOADER_H
#define HEADERLOADER_H

#include "../include/FileLibrary.h"
#include "../include/FileDetails.h"
#include "../include/ToolClasses.hpp"

class FilePath_Loader
{
private:
    fs::path outPutFilePath;
    fs::path filePathToScan;
};
class BinaryIO_Loader
{
};
template <typename T>
T read_binary_le(std::ifstream &file)
{
    T value;
    file.read(reinterpret_cast<char *>(&value), sizeof(T));
    return value;
}

#endif
