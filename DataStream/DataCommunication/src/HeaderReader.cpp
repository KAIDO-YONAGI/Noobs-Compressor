// HeaderReader.cpp
#include "../include/HeaderReader.h"

void BinaryIO::scanner(FilePath &File, FileQueue &queue)
{

    std::ofstream outfile(File.getOutPutFilePath(), std::ios::binary | std::ios::app);
    if (!outfile)
    {
        std::cerr << "scanner()-Error_failToOpenFile:" << File.getFilePathToScan() << "\n";
        return;
    }

    try
    {
        for (auto &entry : fs::directory_iterator(File.getFilePathToScan()))
        {
            std::string name = entry.path().filename().string();

            auto fullPath = entry.path();
            FileNameSize_Int sizeOfName = name.size();
            bool is_File = entry.is_regular_file();
            FileSize_Int fileSize = is_File ? entry.file_size() : 0;

            FileDetails details(name, sizeOfName, fileSize, is_File, fullPath); //创建details
            writeBinaryStandard(outfile, details, queue);
        }
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "scanner()-Error: " << e.what() << "\n";
    }

    outfile.close();
}

void BinaryIO::writeBinaryStandard(std::ofstream &outfile, FileDetails &details, FileQueue &queue)
{
    if (details.getIsFile())
    {
        writeFileStandard(outfile, details);
    }
    else
    {
        FileCount_Int countOfThisHeader = countFilesInDirectory(details.getFullPath());
        if (countOfThisHeader >= 0)
        {

            queue.fileQueue.push({details, countOfThisHeader});
            writeHeaderStandard(outfile, details, countOfThisHeader);
        }
    }
}

void BinaryIO::writeFileStandard(std::ofstream &outfile, FileDetails &details)
{
    FileNameSize_Int SizeOfName = details.getSizeOfName();
    outfile.write("1", 1);                                //先写文件标
    write_binary_le(outfile, SizeOfName);                 //写入文件名偏移量
    outfile.write(details.getName().c_str(), SizeOfName); //写入文件名
    write_binary_le(outfile, details.getFileSize());      //写入文件大小
    write_binary_le(outfile, FileSize_Int(0));            //预留大小
}

void BinaryIO::writeHeaderStandard(std::ofstream &outfile, FileDetails &details, FileCount_Int count)
{
    FileNameSize_Int SizeOfName = details.getSizeOfName();
    outfile.write("0", 1);
    write_binary_le(outfile, SizeOfName);
    outfile.write(details.getName().c_str(), SizeOfName);
    write_binary_le(outfile, count); //写入文件数目
}
void Locator::relativeLocator(std::ofstream &File, FileSize_Int offset)
{
    File.seekp(File.tellp() + static_cast<std::streamoff>(offset), File.beg);
}
void Locator::relativeLocator(std::ifstream &File, FileSize_Int offset)
{
    File.seekg(File.tellg() + static_cast<std::streamoff>(offset), File.beg);
}

void HeaderReader::appendMagicStatic(fs::path &outputFilePath)
{
    std::ofstream outFile(outputFilePath, std::ios::binary | std::ios::app);
    if (!outFile)
    {
        std::cerr << "appendMagicStatic-Error_failToOpenFile: " << outputFilePath << "\n";
        return;
    }

    write_binary_le(outFile, MagicNum);
    outFile.close();
}

void HeaderReader::writeRoot(FilePath &File)
{

    std::ofstream outFile(File.getOutPutFilePath(), std::ios::binary | std::ios::app);
    write_binary_le(outFile, countFilesInDirectory(File.getFilePathToScan()));
    outFile.close();
}
void HeaderReader::scanFlow(FilePath &File)
{
    if (fileIsExist(File.getOutPutFilePath()))
    {
        std::cerr << "scanFlow-Error_fileIsExist \nTry to clear:" << File.getOutPutFilePath() << "\n";
        return;
    }
    FileQueue queue;
    BinaryIO IO;
    appendMagicStatic(File.getOutPutFilePath());

    writeRoot(File); //写入当前根节点的文件数目（若选取多个文件夹，则创建一个根节点）

    IO.scanner(File, queue);

    while (!queue.fileQueue.empty())
    {

        FileDetails &details = (queue.fileQueue.front()).first;
        File.setFilePathToScan(details.getFullPath());
        IO.scanner(File, queue);

        queue.fileQueue.pop();
    }

    appendMagicStatic(File.getOutPutFilePath());
}
MyQueue::MyQueue() : frontNode(nullptr), rearNode(nullptr), count(0) {}

void MyQueue::push(std::pair<FileDetails, FileCount_Int> val)
{
    Node *newNode = new Node(val);
    if (rearNode)
    {
        rearNode->next = newNode;
    }
    else
    {
        frontNode = newNode;
    }
    rearNode = newNode;
    count++;
}
MyQueue::~MyQueue()
{
    while (frontNode)
    { // 循环直到链表为空
        Node *temp = frontNode;
        frontNode = frontNode->next;
        delete temp; // 释放节点内存
    }
    rearNode = nullptr; // 重置尾指针
    count = 0;          // 重置计数器
}
void MyQueue::pop()
{
    if (empty())
    {
        return;
    }
    Node *temp = frontNode;
    frontNode = frontNode->next;
    if (frontNode == nullptr)
    {
        rearNode = nullptr;
    }
    delete temp;
    count--;
}
void readerForCompression()
{
    std::vector<fs::path> tempDirectoryPath;
}

void readerForDecompression()
{
    std::vector<fs::path> tempDirectoryPath;
}

std::pair<FileDetails, FileCount_Int> &MyQueue::front()
{
    if (empty())
    {
        std::cerr << "front()-Error:Queue is empty"
                  << "\n";
    }
    return frontNode->data;
}

std::pair<FileDetails, FileCount_Int> &MyQueue::back()
{
    if (empty())
    {
        std::cerr << "back()-Error:Queue is empty"
                  << "\n";
    }
    return rearNode->data;
}

bool MyQueue::empty()
{
    return count == 0;
}

size_t MyQueue::size()
{
    return count;
}
bool fileIsExist(fs::path &outPutFilePath)
{
    return fs::exists(outPutFilePath);
}

bool isFile(fs::path &outPutFilePath)
{
    return fs::is_regular_file(outPutFilePath);
}

FileSize_Int BinaryIO::getFileSize(fs::path &filePathToScan)
{
    try
    {
        return fs::file_size(filePathToScan);
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "getFileSize()-Error: " << e.what() << "\n";
        return 0;
    }
}

FileCount_Int countFilesInDirectory(fs::path &filePathToScan)
{
    try
    {
        return std::distance(fs::directory_iterator(filePathToScan), fs::directory_iterator{});
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "countFilesInDirectory()-Error: " << e.what() << "\n";
        return -1;
    }
}

// 将字节字符串转换为 Wide 字符串
std::wstring convertToWString(const std::string& s) {
    std::setlocale(LC_ALL, ""); // 使用本地化设置
    size_t len = s.size() + 1;
    wchar_t* wStr = new wchar_t[len];
    size_t result = std::mbstowcs(wStr, s.c_str(), len);
    if (result == (size_t)-1) {
        delete[] wStr;
        throw std::runtime_error("mbstowcs conversion failed");
    }
    std::wstring ws(wStr);
    delete[] wStr;
    return ws;
}

// 处理路径的函数
fs::path _getPath(const std::string& p) {
    std::wstring wPath = convertToWString(p);
    return fs::path(wPath);
}

int main() {
    std::string path;
    std::cout << "Enter a path" << std::endl;
    std::getline(std::cin, path);

    fs::path outPutFilePath = fs::path(L"FilesList.bin"); // 直接使用 Wide 字符串
    fs::path filePathToScan;

    try {
        filePathToScan = _getPath(path); // 调用路径处理函数
        FilePath File(outPutFilePath, filePathToScan);
        HeaderReader reader;
        reader.scanFlow(File);  
    } catch (const std::exception& e) {
        std::cerr << "Main()-Error: " << e.what() << std::endl;
    }

    system("pause");
    return 0;
}
