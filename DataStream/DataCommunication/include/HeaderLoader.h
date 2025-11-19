// HeaderLoader.h
#ifndef HEADERLOADER
#define HEADERLOADER

#include "../include/FileLibrary.h"
#include "../include/FileDetails.h"
#include "../include/ToolClasses.hpp"

class Locator
{
public:
    void relativeLocator(std::ofstream &File, FileSize_Int offset);
    void relativeLocator(std::ifstream &File, FileSize_Int offset);
    void relativeLocator(std::fstream &File, FileSize_Int offset) = delete;
};

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
