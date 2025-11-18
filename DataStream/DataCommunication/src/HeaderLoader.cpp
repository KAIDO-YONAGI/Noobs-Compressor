#include "../include/HeaderLoader.h"

void Locator::relativeLocator(std::ofstream &File, FileSize_Int offset)
{
    File.seekp(File.tellp() + static_cast<std::streamoff>(offset), File.beg);
}
void Locator::relativeLocator(std::ifstream &File, FileSize_Int offset)
{
    File.seekg(File.tellg() + static_cast<std::streamoff>(offset), File.beg);
}

