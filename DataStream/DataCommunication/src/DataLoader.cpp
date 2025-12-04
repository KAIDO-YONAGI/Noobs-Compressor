#include "../include/DataLoader.h"


void DataLoader::dataLoaderForHuffmanCompression(fs::path& inPath)
{
    std::ifstream inFile(inPath, std::ios::binary);
    if (!inFile)
        throw std::runtime_error("dataLoaderForHuffmanCompression()-Error:Failed to open inFile");
}

void dataLoader(){

}
