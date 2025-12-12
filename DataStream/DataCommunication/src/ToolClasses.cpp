#include "../include/ToolClasses.h"

std::wstring Transfer::convertToWString(const std::string &s)
{
    std::setlocale(LC_ALL, ""); // 使用本地化设置
    size_t len = s.size() + 1;
    wchar_t *wStr = new wchar_t[len];
    size_t result = std::mbstowcs(wStr, s.c_str(), len);
    if (result == (size_t)-1)
    {
        delete[] wStr;
        throw std::runtime_error("mbstowcs conversion failed");
    }
    std::wstring ws(wStr);
    delete[] wStr;
    return ws;
}

fs::path Transfer::transPath(const std::string &p)
{
    std::wstring wPath = convertToWString(p);
    return fs::path(wPath);
}

void NumsWriter::appendMagicStatic()
{
    writeBinaryNums(MAGIC_NUM);
}

Directory_FileQueue::Directory_FileQueue() : frontNode(nullptr), rearNode(nullptr), count(0) {}

Directory_FileQueue::~Directory_FileQueue()
{
    clear();
}

void Directory_FileQueue::clear()
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

void Directory_FileQueue::push(std::pair<Directory_FileDetails, FileCount_uint> val)
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
} // 不使用引用，因为使用时会在传值时创建pair，会导致常量引用问题

void Directory_FileQueue::pop()
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

std::pair<Directory_FileDetails, FileCount_uint> &Directory_FileQueue::front()
{
    if (empty())
    {
        throw std::runtime_error("Accessing front of empty Directory_FileQueue");
    }
    return frontNode->data;
}

std::pair<Directory_FileDetails, FileCount_uint> &Directory_FileQueue::back()
{
    if (empty())
    {
        throw std::runtime_error("Accessing back of empty Directory_FileQueue");
    }
    return rearNode->data;
}

bool Directory_FileQueue::empty()
{
    return count == 0;
}

size_t Directory_FileQueue::size()
{
    return count;
}

void Locator::offsetLocator(std::ofstream &outFile, FileSize_uint offset)
{
    outFile.seekp(static_cast<std::streamoff>(offset), outFile.beg);
}

void Locator::offsetLocator(std::ifstream &inFile, FileSize_uint offset)
{
    inFile.seekg(static_cast<std::streamoff>(offset), inFile.beg);
}

FileSize_uint Locator::getFileSize(const fs::path &filePathToScan, std::ofstream &outFile)
{
    try
    {
        outFile.flush();

        return fs::file_size(filePathToScan);
    }
    catch (fs::filesystem_error &e)
    {
        std::cerr << "getFileSize()-Error: " << e.what() << "\n";
        return 0;
    }
}