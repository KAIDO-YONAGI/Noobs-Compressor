#include "../include/DataLoader.h"

namespace Y_flib
{
    void DataLoader::done()
    {
        if (inFile.is_open())
        {
            inFile.close();
        }
        loadIsDone = true;
        readCount = 0;
        data.clear();
    }
    void DataLoader::reset(const std::filesystem::path inPath)
    {
        if (isDone())
        {
            if (inFile.is_open())
            {
                inFile.close();
            }
            inFile = std::ifstream(FileSystemUtils::pathForIo(inPath), std::ios::binary);
            if (!inFile)
                throw std::runtime_error("reset()-Error:Failed to open inFile Path:" + EncodingUtils::pathToUtf8(inPath));
            loadIsDone = false;
        }
        else
            throw std::runtime_error("reset()-Error:inFile is still open, cannot reset to new path:" + EncodingUtils::pathToUtf8(inPath));
    }
    void DataLoader::resetByLastRead()
    {
        Locator locator;
        locator.locateFromBegin(inFile, readCount);
    }
    void DataLoader::dataLoader()
    {
        if (isDone())
            return;
        try
        {
            StandardsReader::readDataBlock(Y_flib::Constants::BUFFER_SIZE, inFile, data);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(
                std::string("DataLoader compression read failed: ") + e.what());
        }

        if (inFile.gcount() == 0)
        {
            done();
        }
        readCount += inFile.gcount();
    }
} // namespace Y_flib
