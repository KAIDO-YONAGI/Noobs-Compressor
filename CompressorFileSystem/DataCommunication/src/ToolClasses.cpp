#include "../include/ToolClasses.h"

std::wstring Transfer::convertToWString(const std::string &utf8_str)
{

    std::wstring wstr;

    size_t i = 0;
    while (i < utf8_str.size())
    {
        uint32_t codepoint = 0;
        uint8_t c = utf8_str[i++];

        // 1字节 UTF-8（0xxxxxxx）
        if ((c & 0x80) == 0)
        {
            codepoint = c;
        }
        // 2字节 UTF-8（110xxxxx 10xxxxxx）
        else if ((c & 0xE0) == 0xC0)
        {
            if (i >= utf8_str.size())
                throw std::runtime_error("Invalid UTF-8: truncated 2-byte sequence");
            uint8_t c2 = utf8_str[i++];
            if ((c2 & 0xC0) != 0x80)
                throw std::runtime_error("Invalid UTF-8: bad continuation byte");
            codepoint = ((c & 0x1F) << 6) | (c2 & 0x3F);
        }
        // 3字节 UTF-8（1110xxxx 10xxxxxx 10xxxxxx）
        else if ((c & 0xF0) == 0xE0)
        {
            if (i + 1 >= utf8_str.size())
                throw std::runtime_error("Invalid UTF-8: truncated 3-byte sequence");
            uint8_t c2 = utf8_str[i++];
            uint8_t c3 = utf8_str[i++];
            if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
            {
                throw std::runtime_error("Invalid UTF-8: bad continuation bytes");
            }
            codepoint = ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
        }
        // 4字节 UTF-8（11110xxx 10xxxxxx 10xxxxxx 10xxxxxx）
        else if ((c & 0xF8) == 0xF0)
        {
            if (i + 2 >= utf8_str.size())
                throw std::runtime_error("Invalid UTF-8: truncated 4-byte sequence");
            uint8_t c2 = utf8_str[i++];
            uint8_t c3 = utf8_str[i++];
            uint8_t c4 = utf8_str[i++];
            if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80)
            {
                throw std::runtime_error("Invalid UTF-8: bad continuation bytes");
            }
            codepoint = ((c & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
        }
        else
        {
            throw std::runtime_error("Invalid UTF-8: illegal leading byte");
        }

        // 转换为 `wchar_t`（自动适配 UTF-16 / UTF-32）
        if (sizeof(wchar_t) == 2)
        {
            // Windows（UTF-16）
            if (codepoint <= 0xFFFF)
            {
                wstr.push_back(static_cast<wchar_t>(codepoint));
            }
            else
            {
                // 代理对（Surrogate Pair）
                codepoint -= 0x10000;
                wstr.push_back(static_cast<wchar_t>(0xD800 | (codepoint >> 10)));
                wstr.push_back(static_cast<wchar_t>(0xDC00 | (codepoint & 0x3FF)));
            }
        }
        else
        {
            // Linux/macOS（UTF-32）
            wstr.push_back(static_cast<wchar_t>(codepoint));
        }
    }

    return wstr;
}

fs::path Transfer::transPath(const std::string &p)
{
    std::wstring wPath = convertToWString(p);
    return fs::path(p);
}

void NumsWriter::appendMagicStatic(std::ofstream &outFile)
{
    writeBinaryNums(MAGIC_NUM, outFile);
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