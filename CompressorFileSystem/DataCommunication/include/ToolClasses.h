// ToolClasses.h
#pragma once

#include "FileLibrary.h"
#include "EntryDetails.h"
#include "EncodingUtils.h"
#include <queue>
#include <fstream>
#include <filesystem>
#include <iostream>

/* EntryQueue - 目录文件队列
 *
 * 功能:
 *   BFS遍历中使用的队列，存储目录和文件计数对
 *   用链表实现，支持push/pop/front/back操作
 */
class EntryQueue : public std::queue<std::pair<EntryDetails, Y_flib::FileCount>>
{
};

/* PathTransfer - 文件路径转换工具（现废弃，暂时使用WINDOWS API）
 *
 * 功能:
 *   为filesystem的fs::path提供宽字符转换支持
 *   解决中文路径问题，处理多字节字符编码转换
 */
class PathTransfer
{
public:
    /* 转换输入路径为fs::path，支持中文路径 */
    std::filesystem::path transPath(std::string_view p);
};

/* Utf8Converter - UTF-8 字符串转换工具
 *
 * 功能:
 *   将 std::u8string 转换为 std::string
 *   用于处理 C++20 中 path::u8string() 返回的 char8_t 类型
 */
class Utf8Converter
{
public:
    /* 将 std::u8string 转换为 std::string */
    static std::string u8_to_string(std::u8string_view u8str);
};

/* StandardsWriter - 二进制数值写入器
 *
 * 功能:
 *   将任意类型数值以二进制格式写入文件流
 *   支持ofstream和fstream，编译时类型检查
 */
class StandardsWriter
{
public:
    /* 模板化函数：将数值写入ofstream（编译时检查可复制性、指针和多态性） */
    template <typename T>
    void writeBinaryStandards(const T value, std::ofstream &ofstream)
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
        if (!ofstream.write(reinterpret_cast<const char *>(&value), sizeof(T))) // 不做类型检查，直接进行类型转换
        {
            throw std::runtime_error("writeBinaryStandards()Error-Failed to write outFile");
        }
    }

    /* 模板化函数：将数值写入fstream（编译时检查可复制性、指针和多态性） */
    template <typename T>
    void writeBinaryStandards(const T value, std::fstream &fstream) // 针对写入fstream的重载
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
        if (!fstream.write(reinterpret_cast<const char *>(&value), sizeof(T))) // 不做类型检查，直接进行类型转换
        {
            throw std::runtime_error("writeBinaryNums()Error-Failed to write inFile");
        }
    }
    void writeBinaryStandards(const std::string &str, std::ofstream &ofstream)
    {
        if (!ofstream)
            throw std::runtime_error("writeBinaryStandards-char*() Error-noOutFile");
        if (!ofstream.write(str.c_str(), str.size()))
        {
            throw std::runtime_error("writeBinaryStandards-char*()Error-Failed to write outFile");
        }
    }

    static void writeDataBlock(Y_flib::FileSize size, std::ofstream &file, const Y_flib::DataBlock &buffer)
    {
        if (!file.write(reinterpret_cast<const char *>(buffer.data()), size))
        {
            throw std::runtime_error("writeDataBlock(std::ofstream)-Failed to write header");
        }
    }
    static void writeDataBlock(Y_flib::FileSize size, std::fstream &file, const Y_flib::DataBlock &buffer)
    {
        if (!file.write(reinterpret_cast<const char *>(buffer.data()), size))
        {
            throw std::runtime_error("writeDataBlock(std::fstream &file)-Failed to write header");
        }
    }
    /* 写入静态魔数标记到输出文件 */
    void appendMagicStatic(std::ofstream &outFile)
    {
        writeBinaryStandards(Y_flib::Constants::MAGIC_NUM, outFile);
    }
};

/* StandardsReader - 二进制数值读取器
 *
 * 功能:
 *   从文件流中读取二进制格式的数值
 *   支持任意平凡可复制的类型，编译时类型检查
 */
class StandardsReader
{
private:
    std::istream &file;

public:
    StandardsReader(std::istream &file) : file(file) {}

    ~StandardsReader() = default;

    /* 读取平凡类型 */
    template <typename T>
    T readBinaryStandards()
    {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Cannot read non-trivially-copyable type");
        static_assert(!std::is_pointer_v<T>,
                      "Cannot safely read raw pointers");
        static_assert(!std::is_polymorphic_v<T>,
                      "Cannot safely read polymorphic types");

        T value{};

        file.read(reinterpret_cast<char *>(&value), sizeof(T));

        if (file.gcount() != sizeof(T))
        {
            throw std::runtime_error("readBinaryStandards: unexpected EOF");
        }

        return value;
    }
    /* 读取数据块 */
    static std::streamsize readDataBlock(
        Y_flib::FileSize size,
        std::istream &file,
        Y_flib::DataBlock &buffer)
    {
        buffer.resize(size);

        file.read(reinterpret_cast<char *>(buffer.data()), size);

        std::streamsize n = file.gcount();
        // if (file.gcount() < size)
        // {
        //     throw std::runtime_error("readDataBlock: unexpected EOF-- expected " + std::to_string(size) + " bytes, got " + std::to_string(n));
        // }
        if (file.bad())
        {
            throw std::runtime_error("readDataBlock: read error");
        }

        buffer.resize(static_cast<size_t>(n));

        return n;
    }
};
/* EntryQueue - 目录文件队列
 *
 * 功能:
 *   BFS遍历中使用的队列，存储目录和文件计数对
 *   用链表实现，支持push/pop/front/back操作
 */

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
