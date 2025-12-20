// Directory_FileWriter.h
#pragma once

#include "FileLibrary.h"
#include "Directory_FileProcessor.h"

/* HeaderWriter_Interface - 文件头和目录写入的抽象接口
 *
 * 功能:
 *   定义文件头写入和目录结构序列化的标准接口
 *   支持多版本实现的运行时切换
 *   分离头部格式和目录写入的具体实现
 */
class HeaderWriter_Interface
{
public:
    virtual ~HeaderWriter_Interface() = default;

    /* 写入文件头信息到输出文件 */
    virtual void writeHeader(std::ofstream &outFile, fs::path &fullOutPath) = 0;

    /* 序列化目录结构并写入输出文件 */
    virtual void writeDirectory(
        std::ofstream &outFile,
        const  std::vector<std::string> &filePathToScan,
        const fs::path &fullOutPath,
        const std::string &logicalRoot) = 0;
};

/* HeaderWriter_v0 - 文件头和目录写入的具体实现（v0版本）
 *
 * 功能:
 *   实现HeaderWriter_Interface的v0版本
 *   处理文件头初始化和目录二进制序列化
 */
class HeaderWriter_v0 : public HeaderWriter_Interface
{
public:
    HeaderWriter_v0() = default;

    /* 写入v0格式的文件头 */
    void writeHeader(std::ofstream &outFile, fs::path &fullOutPath) override;

    /* 写入v0格式的目录结构 */
    void writeDirectory(
        std::ofstream &outFile,
        const  std::vector<std::string> &filePathToScan,
        const fs::path &fullOutPath,
        const std::string &logicalRoot) override;
};

/* HeaderWriter - 文件头写入的策略模式包装类
 *
 * 功能:
 *   通过策略模式支持多版本文件头格式
 *   提供统一的写入接口，隐藏具体实现
 *   允许运行时切换不同的写入策略
 */
class HeaderWriter
{
    std::unique_ptr<HeaderWriter_Interface> writer; // 智能指针，支持运行时切换模式

public:
    /* 构造函数，默认使用v0版本 */
    HeaderWriter() : writer(std::make_unique<HeaderWriter_v0>()) {}

    /* 支持指定版本的构造函数 */
    explicit HeaderWriter(std::unique_ptr<HeaderWriter_Interface> impl)
        : writer(std::move(impl)) {}

    /* 主写入函数，统一处理文件头和目录结构的序列化 */
    void headerWriter(const std::vector<std::string> &filePathToScan, std::string &outPutFilePath, const std::string &logicalRoot);

    /* 委托写入文件头到指定策略实现 */
    void writeHeader(std::ofstream &outFile, fs::path &fullOutPath)
    {
        writer->writeHeader(outFile, fullOutPath);
    }

    /* 委托写入目录结构到指定策略实现 */
    void writeDirectory(std::ofstream &outFile, const  std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot)
    {
        writer->writeDirectory(outFile, filePathToScan, fullOutPath, logicalRoot);
    };
};
