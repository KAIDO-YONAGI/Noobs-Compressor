// DataExporter.h
#pragma once

#include "../include/FileLibrary.h"

void exportDataToFile(const std::vector<char> &data, const fs::path &outPath)
{
    std::ofstream outFile(outPath, std::ios::binary);
    if (!outFile)        
    {
        throw std::runtime_error("exportDataToFile()-Error:Failed to open outFile");
    }
    
    outFile.write(data.data(), data.size());
    outFile.close();
}