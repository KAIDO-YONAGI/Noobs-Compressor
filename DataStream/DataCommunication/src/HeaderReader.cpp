// HeaderReader.cpp
#include "../include/HeaderReader.h"

void readerForCompression(){
    std::vector<fs::path> tempDirectoryPath;
}

void readerForDecompression(){
    std::vector<fs::path> tempDirectoryPath;
}

void scanFlow(FilePath &File)
{
    if (fileIsExist(File.getOutPutFilePath()))
    {
        std::cerr << "scanFlow-Error_fileIsExist \nTry to clear:" << File.getOutPutFilePath() << "\n";
        return;
    }
    
    BinaryIO headerReader(File);
    appendMagicStatic(File.getOutPutFilePath());
    headerReader.scanner();
    appendMagicStatic(File.getOutPutFilePath());
}

bool fileIsExist(const fs::path &outPutFilePath)
{
    return fs::exists(outPutFilePath);
}

bool isFile(const fs::path &outPutFilePath)
{
    return fs::is_regular_file(outPutFilePath);
}

void appendMagicStatic(const fs::path &outputFilePath)
{
    std::ofstream outFile(outputFilePath, std::ios::binary | std::ios::app);
    if (!outFile)
    {
        std::cerr << "appendMagicStatic-Error_failToOpenFile: " << outputFilePath << "\n";
        return;
    }

    const uint32_t magic = 0xDEADBEEF;
    outFile.write(reinterpret_cast<const char*>(&magic), sizeof(magic));

    if (!outFile)
    {
        std::cerr << "Error_Can't write magic num in file\n";
    }
}

uint64_t BinaryIO::getFileSize(const fs::path &filePathToScan)
{
    try {
        return fs::file_size(filePathToScan);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "getFileSize()-Error: " << e.what() << "\n";
        return 0;
    }
}

void BinaryIO::scanner()
{
    FileQueue queue;
    bool goingScan = true;
    
    while (true)
    {
        std::ofstream outfile(File.getOutPutFilePath(), std::ios::binary | std::ios::app);
        if (!outfile)
        {
            std::cerr << "scanner-Error_failToOpenFile:" << File.getFilePathToScan() << "\n";
            return;
        }

        goingScan = false;
        
        try {
            for (const auto& entry : fs::directory_iterator(File.getFilePathToScan()))
            {
                goingScan = true;
                auto name = entry.path().filename().string();
                if (name == "." || name == "..")
                    continue;
                    
                auto fullPath = entry.path();
                uint8_t sizeOfName = name.size();
                bool is_File = entry.is_regular_file();
                uint64_t fileSize = is_File ? entry.file_size() : 0;

                FileDetails details(name, sizeOfName, fileSize, is_File, fullPath);
                writeBinaryStandard(outfile, details, queue);
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "scanner-Error: " << e.what() << "\n";
            continue;
        }

        if(!goingScan && queue.fileQueue.empty()) break;
        
        outfile.close();
    }
}

void BinaryIO::writeBinaryStandard(std::ofstream &outfile, FileDetails &details, FileQueue &queue)
{
    if (details.getIsFile())
    {
        writeFileStandard(outfile, details);
    }
    else
    {
        int countOfThisHeader = countFilesInDirectory(details.getFullPath());
        if(countOfThisHeader >= 0){
            queue.fileQueue.push({details, countOfThisHeader});
            writeHeaderStandard(outfile, details, countOfThisHeader);
        }
    }
}

void BinaryIO::writeFileStandard(std::ofstream &outfile, FileDetails &details)
{
    uint8_t SizeOfName = details.getSizeOfName();
    write_binary_le(outfile, SizeOfName);                 
    outfile.write(details.getName().c_str(), SizeOfName); 
    outfile.write("1", 1);                                
    write_binary_le(outfile, details.getFileSize());      
    write_binary_le(outfile, uint64_t(0));                
}

void BinaryIO::writeHeaderStandard(std::ofstream &outfile, FileDetails &details, uint32_t count)
{
    uint8_t SizeOfName = details.getSizeOfName();
    write_binary_le(outfile, SizeOfName);                 
    outfile.write(details.getName().c_str(), SizeOfName); 
    outfile.write("0", 1);
    write_binary_le(outfile, count); 
}

int countFilesInDirectory(const fs::path &filePathToScan)
{
    try {
        return std::distance(fs::directory_iterator(filePathToScan), fs::directory_iterator{});
    } catch (const fs::filesystem_error& e) {
        std::cerr << "writeHeaderStandard()-Error: " << e.what() << "\n";
        return -1;
    }
}

int main()
{
    fs::path outPutFilePath = "FilesList.bin";
    fs::path filePathToScan = "D:/1gal";

    FilePath File(outPutFilePath, filePathToScan);
    scanFlow(File);

    system("pause");
    return 0;
}
