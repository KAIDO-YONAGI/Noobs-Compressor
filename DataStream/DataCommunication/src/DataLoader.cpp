#include "../include/DataLoader.h"

std::vector<char> DataLoader::dataLoader()
{
    std::vector<char> buffer(BUFFER_SIZE);
    try
    {
        inFile.read(buffer.data(), BUFFER_SIZE);
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
