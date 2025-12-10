// ToolClasses.hpp
#pragma once

#include "../include/FileLibrary.h"
#include "../include/Directory_FileDetails.h"
/*
Transfer类：为filesystem的fs::path类型提供宽字符转换方案（主要处理中文路径问题），且仅在自行创建fs::path时使用，用于输入需要SFC处理的路径
MagicNumWriter类：写魔数的
Directory_FileQueue：目录路径存取特化的队列
Locator：使用偏移量的定位器
*/
class Transfer
{
public:
    std::wstring convertToWString(const std::string &s)
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

    fs::path transPath(const std::string &p)
    {
        std::wstring wPath = convertToWString(p);
        return fs::path(wPath);
    }
};

class NumsWriter
{
private:
    std::ofstream &file;

public:
    NumsWriter(std::ofstream &file) : file(file) {};

    template <typename T>
    void writeBinaryNums(T value)
    {
        if (!file)
            throw std::runtime_error("writeBinaryNums() Error-noFile");
        // 编译时检查

        static_assert(std::is_trivially_copyable_v<T>,
                      "Cannot write non-trivially-copyable type");
        static_assert(!std::is_pointer_v<T>,
                      "Cannot safely write raw pointers");
        static_assert(!std::is_polymorphic_v<T>,
                      "Cannot safely write polymorphic types");
        if (!file.write(reinterpret_cast<char *>(&value), sizeof(T))) // 不做类型检查，直接进行类型转换
        {
            throw std::runtime_error("writeBinaryNums()Error-Failed to write");
        }
    }

    void appendMagicStatic()
    {
        writeBinaryNums(MAGIC_NUM);
    }
};
class NumsReader
{
private:
    std::ifstream &file;

public:
    NumsReader(std::ifstream &file) : file(file) {};
    template <typename T>
    T readBinaryNums()
    {
        if (!file)
            throw std::runtime_error("readBinaryNums() Error-noFile");
        T value;
        static_assert(std::is_trivially_copyable_v<T>,
                      "Cannot write non-trivially-copyable type");
        static_assert(!std::is_pointer_v<T>,
                      "Cannot safely write raw pointers");
        static_assert(!std::is_polymorphic_v<T>,
                      "Cannot safely write polymorphic types");

        if (!file.read(reinterpret_cast<char *>(&value), sizeof(T)))
        {
            throw std::runtime_error("readBinaryNums()Error-Failed to read");
        }
        return value;
    }
};

class Directory_FileQueue
{
private:
    struct Node
    {
        std::pair<Directory_FileDetails, FileCount_uint> data;
        Node *next;
        Node(const std::pair<const Directory_FileDetails, FileCount_uint> &val)
            : data(val), next(nullptr) {}
    };

    Node *frontNode;
    Node *rearNode;
    size_t count;

public:
    Directory_FileQueue() : frontNode(nullptr), rearNode(nullptr), count(0) {}
    ~Directory_FileQueue()
    {
        clear();
    }
    void clear()
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
    void push(std::pair<Directory_FileDetails, FileCount_uint> val)
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
    void pop()
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

    std::pair<Directory_FileDetails, FileCount_uint> &front()
    {
        if (empty())
        {
            throw std::runtime_error("Accessing front of empty directoryQueue");
        }
        return frontNode->data;
    }

    std::pair<Directory_FileDetails, FileCount_uint> &back()
    {
        if (empty())
        {
            throw std::runtime_error("Accessing back of empty directoryQueue");
        }
        return rearNode->data;
    }

    bool empty()
    {
        return count == 0;
    }

    size_t size()
    {
        return count;
    }
};
class directoryQueueInterface
{
public:
    Directory_FileQueue Directory_FileQueue;
};

class Locator
{
public:
    Locator() = default;
    void offsetLocator(std::ofstream &outFile, FileSize_uint offset)
    {
        outFile.seekp(static_cast<std::streamoff>(offset), outFile.beg);
    }
    void offsetLocator(std::ifstream &inFile, FileSize_uint offset)
    {
        inFile.seekg(static_cast<std::streamoff>(offset), inFile.beg);
    }

    void offsetLocator(std::fstream &file, FileSize_uint offset) = delete;

    FileSize_uint getFileSize(const fs::path &filePathToScan, std::ofstream &outFile)
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
};
