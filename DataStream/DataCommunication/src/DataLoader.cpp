#include "../include/DataLoader.h"

std::vector<char> DataLoader::dataLoader()
{
    std::vector<char> buffer(BufferSize);
    try
    {
        inFile.read(buffer.data(), BufferSize);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    if (buffer.size() == 0)
    {
        done();
    }
    return buffer;
}
