// ToolClasses.h
#pragma once

#include "FileLibrary.h"
#include "EntryDetails.h"
#include "EncodingUtils.h"
#include <queue>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace Y_flib
{

    /* EntryQueue - 目录文件队列
     *
     * 功能:
     *   BFS遍历中使用的队列，存储目录和子项计数对（second 语义单一：子项数）
     *   用链表实现，支持push/pop/front/back操作
     */
    using EntryQueue = std::queue<std::pair<EntryDetails, Y_flib::FileCount>>;

    /* FileTask - 待处理文件任务
     *
     * 功能:
     *   压缩/解压流程中 fileQueue 的元素。
     *   消灭原 pair.second 双语义：压缩模式装"处理后大小"预留槽偏移，
     *   解压模式装压缩后大小——两种载荷各自具名，按模式取用
     */
    struct FileTask
    {
        EntryDetails entry;                 // 待处理文件的目录条目
        Y_flib::SlotOffset processedSizeOffset = 0; // 压缩模式：预留字段在归档中的偏移
        Y_flib::FileSize compressedSize = 0;        // 解压模式：该文件压缩后的大小
    };

    using FileTaskQueue = std::queue<FileTask>;

    /* LinkTask - 解压末尾统一重建的 Windows 链接任务
     *
     * 归档只保存链接类型、名称和链接自身记录的目标路径。
     * 目标可以是相对路径、绝对路径或不存在的路径，解压端不负责解析。
     */
    struct LinkTask
    {
        Y_flib::FlagType linkType = Y_flib::FlagType::SymbolicLinkFile;
        std::filesystem::path linkPath;
        std::filesystem::path targetPath;
    };

    using LinkTaskQueue = std::queue<LinkTask>;

    /* StandardsWriter - 二进制数值写入器
     *
     * 功能:
     *   将任意类型数值以二进制格式写入文件流
     *   统一接收 std::ostream，因此 ofstream 和 fstream 共用同一套写入逻辑
     */
    class StandardsWriter
    {
    public:
        /* 写入平凡可复制类型；归档格式直接由 T 的固定宽度决定。 */
        template <typename T>
        void writeBinaryStandards(const T value, std::ostream &stream)
        {
            if (!stream)
                throw std::runtime_error("writeBinaryStandards(): invalid output stream");

            static_assert(std::is_trivially_copyable_v<T>,
                          "Cannot write non-trivially-copyable type");
            static_assert(!std::is_pointer_v<T>,
                          "Cannot safely write raw pointers");
            static_assert(!std::is_polymorphic_v<T>,
                          "Cannot safely write polymorphic types");
            if (!stream.write(reinterpret_cast<const char *>(&value), sizeof(T)))
            {
                throw std::runtime_error("writeBinaryStandards(): failed to write output stream");
            }
        }

        /* 字符串按原始字节写入，长度字段由调用方按照归档布局单独写入。 */
        void writeBinaryStandards(const std::string &str, std::ostream &stream)
        {
            if (!stream)
                throw std::runtime_error("writeBinaryStandards(string): invalid output stream");
            if (!stream.write(str.data(), static_cast<std::streamsize>(str.size())))
            {
                throw std::runtime_error("writeBinaryStandards(string): failed to write output stream");
            }
        }

        /* 数据块只负责写正文，调用方负责保证 size 不超过 buffer.size()。 */
        static void writeDataBlock(
            Y_flib::FileSize size,
            std::ostream &stream,
            const Y_flib::DataBlock &buffer)
        {
            if (!stream.write(
                    reinterpret_cast<const char *>(buffer.data()),
                    static_cast<std::streamsize>(size)))
            {
                throw std::runtime_error("writeDataBlock(): failed to write output stream");
            }
        }

        /* 写入静态魔数标记到输出文件 */
        void appendMagicStatic(std::ostream &stream)
        {
            writeBinaryStandards(Y_flib::Constants::MAGIC_NUM, stream);
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
    };
} // namespace Y_flib
