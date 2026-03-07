#include "../include/ToolClasses.h"
namespace fs = std::filesystem;

std::wstring PathTransfer::convertToWString(const std::string &s)
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

std::filesystem::path PathTransfer::transPath(const std::string &p)
{
    return std::filesystem::path(p);
}

void StandardsWriter::appendMagicStatic(std::ofstream &outFile)
{
    writeBinaryStandards(MAGIC_NUM, outFile);
}

void EntryQueue::clear()
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

void EntryQueue::push(std::pair<EntryDetails, Y_flib::FileCount> val)
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

void EntryQueue::pop()
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
//返回头部的EntryDetails对象的引用，允许直接访问和修改队头元素的数据
std::pair<EntryDetails, Y_flib::FileCount> &EntryQueue::front()
{
    if (empty())
    {
        throw std::runtime_error("Accessing front of empty EntryQueue");
    }
    return frontNode->data;
}
//返回尾部的EntryDetails对象的引用，允许直接访问和修改队尾元素的数据
std::pair<EntryDetails, Y_flib::FileCount> &EntryQueue::back()
{
    if (empty())
    {
        throw std::runtime_error("Accessing back of empty EntryQueue");
    }
    return rearNode->data;
}

bool EntryQueue::empty()
{
    return count == 0;
}

size_t EntryQueue::size()
{
    return count;
}

void Locator::locateFromBegin(std::ofstream &outFile, Y_flib::FileSize offset)
{
    try
    {
        if (!outFile)
        {
            throw std::runtime_error("locateFromBegin() Error-noOutFile");
        }
        else
        {
            outFile.clear(); // 清除错误标志，确保seekp成功
            outFile.flush(); // 确保之前的写入已完成
            outFile.seekp(static_cast<std::streamoff>(offset), outFile.beg);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return;
    }
}

void Locator::locateFromBegin(std::ifstream &inFile, Y_flib::FileSize offset)
{
    try
    {
        if (!inFile)
        {
            throw std::runtime_error("locateFromBegin() Error-noInFile");
        }
        else
        {
            inFile.clear(); // 清除错误标志，确保seekp成功
            inFile.seekg(static_cast<std::streamoff>(offset), inFile.beg);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return;
    }
}
void Locator::locateFromEnd(std::ofstream &outFile, Y_flib::FileSize offset)
{
    try
    {
        if (!outFile)
        {
            throw std::runtime_error("locateFromEnd() Error-noOutFile");
        }
        else
        {
            outFile.clear();  // 清除错误标志，确保seekp成功
            outFile.flush(); // 确保之前的写入已完成
            outFile.seekp(static_cast<std::streamoff>(offset), outFile.end);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return;
    }
}

void Locator::locateFromEnd(std::ifstream &inFile, Y_flib::FileSize offset)
{
    try
    {
        if (!inFile)
        {
            throw std::runtime_error("locateFromEnd() Error-noInFile");
        }
        else
        {
            inFile.clear(); // 清除错误标志，确保seekp成功
            inFile.seekg(static_cast<std::streamoff>(offset), inFile.end);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return;
    }
}
void Locator::locateFromBegin(std::fstream &file, Y_flib::FileSize offset)
{
    try
    {
        if (!file)
        {
            throw std::runtime_error("locateFromBegin-fstream() Error-noFile");
        }
        else
        {

            file.clear(); // 清除错误标志，确保seekp成功
            file.flush();
            file.seekg(offset, std::ios::beg); // 读
            file.seekp(offset, std::ios::beg); // 写
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return;
    }
}
void Locator::locateFromEnd(std::fstream &file, Y_flib::FileSize offset)
{
    try
    {
        if (!file)
        {
            throw std::runtime_error("locateFromBegin-fstream() Error-noFile");
        }
        else
        {
            file.clear(); // 清除错误标志，确保seekp成功
            file.flush();
            file.seekg(offset, std::ios::end); // 读
            file.seekp(offset, std::ios::end); // 写
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return;
    }
}
Y_flib::FileSize Locator::getFileSize(const fs::path &filePathToScan, std::ofstream &outFile)
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