// ToolClasses.h
#ifndef TOOLCLASSES
#define TOOLCLASSES

#include "../include/FileLibrary.h"
#include "../include/FileDetails.h"

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

    fs::path _getPath(const std::string &p)
    {
        std::wstring wPath = convertToWString(p);
        return fs::path(wPath);
    }
};

class MagicNumWriter
{
public:
    template <typename T>
    void write_binary_le(std::ofstream &file, T value)
    {
        file.write(reinterpret_cast<char *>(&value), sizeof(T)); //不做类型检查，直接进行类型转换
    }
    void appendMagicStatic(const fs::path &outputFilePath)
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
};

class FileQueue
{
private:
    struct Node
    {
        std::pair<FileDetails, FileCount_uint> data;
        Node *next;
        Node(const std::pair<const FileDetails, FileCount_uint> &val)
            : data(val), next(nullptr) {}
    };

    Node *frontNode;
    Node *rearNode;
    size_t count;

public:
    FileQueue() : frontNode(nullptr), rearNode(nullptr), count(0) {}
    ~FileQueue()
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
    void push(std::pair<FileDetails, FileCount_uint> val)
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

    std::pair<FileDetails, FileCount_uint> &front()
    {
        if (empty())
        {
            throw std::runtime_error("Accessing front of empty queue");
        }
        return frontNode->data;
    }

    std::pair<FileDetails, FileCount_uint> &back()
    {
        if (empty())
        {
            throw std::runtime_error("Accessing back of empty queue");
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
class QueueInterface
{
public:
    FileQueue fileQueue;
};

class Locator
{
public:
    Locator() = default;
    void offsetLocator(std::ofstream &File, FileSize_uint offset)
    {
        File.seekp(static_cast<std::streamoff>(offset), File.beg);
    }
    void offsetLocator(std::ifstream &File, FileSize_uint offset)
    {
        File.seekg(static_cast<std::streamoff>(offset), File.beg);
    }

    void offsetLocator(std::fstream &File, FileSize_uint offset) = delete;

    FileSize_uint getFileSize(fs::path &filePathToScan)
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
};

#endif