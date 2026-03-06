// ToolClasses.h
#pragma once

#include "FileLibrary.h"
#include "EntryDetails.h"

/* PathTransfer - 文件路径转换工具（现废弃，暂时使用WINDOWS API）
 *
 * 功能:
 *   为filesystem的fs::path提供宽字符转换支持
 *   解决中文路径问题，处理多字节字符编码转换
 */
class PathTransfer
{
private:
    /* 将std::string转换为std::wstring，处理编码转换 */
    std::wstring convertToWString(const std::string &s);

public:
    /* 转换输入路径为fs::path，支持中文路径 */
    std::filesystem::path transPath(const std::string &p);
};

/* StandardWriter - 二进制数值写入器
 *
 * 功能:
 *   将任意类型数值以二进制格式写入文件流
 *   支持ofstream和fstream，编译时类型检查
 */
class StandardWriter
{
public:
    /* 模板化函数：将数值写入ofstream（编译时检查可复制性、指针和多态性） */
    template <typename T>
    void writeBinaryStandards(T value, std::ofstream &ofstream)
    {
        if (!ofstream)
            throw std::runtime_error("writeBinaryStandards() Error-noOutFile");
        // 编译时检查

        static_assert(std::is_trivially_copyable_v<T>,
                      "Cannot write non-trivially-copyable type");
        static_assert(!std::is_pointer_v<T>,
                      "Cannot safely write raw pointers");
        static_assert(!std::is_polymorphic_v<T>,
                      "Cannot safely write polymorphic types");
        if (!ofstream.write(reinterpret_cast<char *>(&value), sizeof(T))) // 不做类型检查，直接进行类型转换
        {
            throw std::runtime_error("writeBinaryStandards()Error-Failed to write outFile");
        }

    }

    /* 模板化函数：将数值写入fstream（编译时检查可复制性、指针和多态性） */
    template <typename T>
    void writeBinaryStandards(T value, std::fstream &fstream) // 针对写入fstream的重载
    {
        if (!fstream)
            throw std::runtime_error("writeBinaryNums() Error-noInFile");
        // 编译时检查

        static_assert(std::is_trivially_copyable_v<T>,
                      "Cannot write non-trivially-copyable type");
        static_assert(!std::is_pointer_v<T>,
                      "Cannot safely write raw pointers");
        static_assert(!std::is_polymorphic_v<T>,
                      "Cannot safely write polymorphic types");
        if (!fstream.write(reinterpret_cast<char *>(&value), sizeof(T))) // 不做类型检查，直接进行类型转换
        {
            throw std::runtime_error("writeBinaryNums()Error-Failed to write inFile");
        }

    }
    void writeBinaryStandards(const std::string str, std::ofstream &ofstream)
    {
        if (!ofstream)
            throw std::runtime_error("writeBinaryStandards-char*() Error-noOutFile");
        if (!ofstream.write(str.c_str(), str.size()))
        {
            throw std::runtime_error("writeBinaryStandards-char*()Error-Failed to write outFile");
        }

    }

    /* 写入静态魔数标记到输出文件 */
    void appendMagicStatic(std::ofstream &outFile);
};

/* BinaryStandardsReader - 二进制数值读取器
 *
 * 功能:
 *   从文件流中读取二进制格式的数值
 *   支持任意平凡可复制的类型，编译时类型检查
 */
class BinaryStandardsReader
{
private:
    std::ifstream &file;

public:
    /* 构造函数，关联输入文件流 */
    BinaryStandardsReader(std::ifstream &file) : file(file) {};

    /* 默认析构函数 */
    ~BinaryStandardsReader() = default;

    /* 模板化函数：从文件读取指定类型的数值（编译时检查可复制性） */
    template <typename T>
    T readBinaryStandards()
    {
        if (!file)
            throw std::runtime_error("readBinaryStandards() Error-noFile");
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
            std::string msg = std::string("readBinaryStandards()Error-Failed to read") + (file.eof() ? " - End of File reached" : "");
            throw std::runtime_error(msg);
        }
        return value;
    }
};
template <typename T>
class MyQueueInterface
{
public:
    virtual ~MyQueueInterface() = default;

    virtual void push(T) = 0;
    virtual void pop() = 0;
    virtual bool empty() = 0;
    virtual T &front() = 0;
    virtual T &back() = 0;
    virtual size_t size() = 0;
    virtual void clear() = 0;
};
/* EntryQueue - 目录文件队列
 *
 * 功能:
 *   BFS遍历中使用的队列，存储目录和文件计数对
 *   用链表实现，支持push/pop/front/back操作
 */
class EntryQueue : public MyQueueInterface<std::pair<EntryDetails, Y_flib::FileCount>>
{
private:
    struct Node
    {
        std::pair<EntryDetails, Y_flib::FileCount> data;
        Node *next;
        Node(const std::pair<EntryDetails, Y_flib::FileCount> &val)
            : data(val), next(nullptr) {}
    };

    Node *frontNode;
    Node *rearNode;
    size_t count;

public:
    EntryQueue() : frontNode(nullptr), rearNode(nullptr), count(0) {}

    ~EntryQueue()
    {
        clear();
    }

    /* 清空队列中的所有元素 */
    void clear() override;

    /* 入队（push_back），添加元素到队尾 */
    void push(std::pair<EntryDetails, Y_flib::FileCount> val) override;

    /* 出队（pop_front），移除队头元素 */
    void pop() override;

    /* 检查队列是否为空 */
    bool empty() override;
    /* 获取队头元素引用 */
    std::pair<EntryDetails, Y_flib::FileCount> &front() override;

    /* 获取队尾元素引用 */
    std::pair<EntryDetails, Y_flib::FileCount> &back() override;

    /* 获取队列中的元素个数 */
    size_t size() override;
};

/* Locator - 文件位置定位器
 *
 * 功能:
 *   使用偏移量定位文件中的特定位置
 *   支持输入/输出文件流的随机访问
 */
class Locator
{
public:
    /* 默认构造函数 */
    Locator() = default;

    /* 在输出文件中定位到指定偏移位置 */
    void locateFromBegin(std::ofstream &outFile, Y_flib::FileSize offset);
    void locateFromEnd(std::ofstream &outFile, Y_flib::FileSize offset);

    /* 在输入文件中定位到指定偏移位置 */
    void locateFromBegin(std::ifstream &inFile, Y_flib::FileSize offset);
    void locateFromEnd(std::ifstream &inFile, Y_flib::FileSize offset);

    void locateFromBegin(std::fstream &file, Y_flib::FileSize offset);
    void locateFromEnd(std::fstream &file, Y_flib::FileSize offset);
    /* 获取输出文件的当前大小 */
    Y_flib::FileSize getFileSize(const std::filesystem::path &filePathToScan, std::ofstream &outFile);
};