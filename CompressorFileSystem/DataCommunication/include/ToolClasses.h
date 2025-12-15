// ToolClasses.h
#pragma once

#include "FileLibrary.h"
#include "Directory_FileDetails.h"
/*
Transfer类：为filesystem的fs::path类型提供宽字符转换方案（主要处理中文路径问题），且仅在自行创建fs::path时使用，用于输入需要SFC处理的路径
MagicNumWriter类：写魔数的
Directory_FileQueue：目录路径存取特化的队列
Locator：使用偏移量的定位器
*/
class Transfer
{
private:
    std::wstring convertToWString(const std::string &s);

public:
    fs::path transPath(const std::string &p);
};

class NumsWriter
{
public:
    template <typename T>
    void writeBinaryNums(T value, std::ofstream &ofstream)
    {
        if (!ofstream)
            throw std::runtime_error("writeBinaryNums() Error-noFile");
        // 编译时检查

        static_assert(std::is_trivially_copyable_v<T>,
                      "Cannot write non-trivially-copyable type");
        static_assert(!std::is_pointer_v<T>,
                      "Cannot safely write raw pointers");
        static_assert(!std::is_polymorphic_v<T>,
                      "Cannot safely write polymorphic types");
        if (!ofstream.write(reinterpret_cast<char *>(&value), sizeof(T))) // 不做类型检查，直接进行类型转换
        {
            throw std::runtime_error("writeBinaryNums()Error-Failed to write");
        }
    }
    template <typename T>
    void writeBinaryNums(T value, std::fstream &fstream) // 针对写入fstream的重载
    {
        if (!fstream)
            throw std::runtime_error("1writeBinaryNums() Error-noFile");
        // 编译时检查

        static_assert(std::is_trivially_copyable_v<T>,
                      "Cannot write non-trivially-copyable type");
        static_assert(!std::is_pointer_v<T>,
                      "Cannot safely write raw pointers");
        static_assert(!std::is_polymorphic_v<T>,
                      "Cannot safely write polymorphic types");
        if (!fstream.write(reinterpret_cast<char *>(&value), sizeof(T))) // 不做类型检查，直接进行类型转换
        {
            throw std::runtime_error("1writeBinaryNums()Error-Failed to write");
        }
    }

    void appendMagicStatic(std::ofstream &outFile);
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

        if (file.eof())
        {
            return T();
        }
        if (!file.read(reinterpret_cast<char *>(&value), sizeof(T)))
        {
            std::string msg = std::string("readBinaryNums()Error-Failed to read") + (file.eof() ? " - End of File reached" : "");
            throw std::runtime_error(msg);
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
        Node(const std::pair<Directory_FileDetails, FileCount_uint> &val)
            : data(val), next(nullptr) {}
    };

    Node *frontNode;
    Node *rearNode;
    size_t count;

public:
    Directory_FileQueue();
    ~Directory_FileQueue();
    void clear();
    void push(std::pair<Directory_FileDetails, FileCount_uint> val);
    void pop();
    std::pair<Directory_FileDetails, FileCount_uint> &front();
    std::pair<Directory_FileDetails, FileCount_uint> &back();
    bool empty();
    size_t size();
};

class Directory_FIleQueueInterface
{
public:
    Directory_FileQueue Directory_FileQueue;
};

class Locator
{
public:
    Locator() = default;
    void offsetLocator(std::ofstream &outFile, FileSize_uint offset);
    void offsetLocator(std::ifstream &inFile, FileSize_uint offset);
    void offsetLocator(std::fstream &file, FileSize_uint offset) = delete;
    FileSize_uint getFileSize(const fs::path &filePathToScan, std::ofstream &outFile);
};