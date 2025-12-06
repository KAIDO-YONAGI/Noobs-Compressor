// Directory_FileWriter.h
#pragma once

#include "../include/FileLibrary.h"
#include "../include/Directory_FileProcessor.h"

class HeaderWriter_Interface
{
public:
    virtual ~HeaderWriter_Interface() = default;
    virtual void writeHeader(std::ofstream &outFile, fs::path &fullOutPath) = 0; //=0:标明是纯虚函数，必须重写，没有=0则可选择性重写
    virtual void writeDirectory(
        std::ofstream &outFile,
        const std::vector<std::string> &filePathToScan,
        const fs::path &fullOutPath,
        const std::string &logicalRoot) = 0;
};

class HeaderWriter_v0 : public HeaderWriter_Interface
{
public:
    HeaderWriter_v0() = default;
    void writeHeader(std::ofstream &outFile, fs::path &fullOutPath) override;
    void writeDirectory(
        std::ofstream &outFile,
        const std::vector<std::string> &filePathToScan,
        const fs::path &fullOutPath,
        const std::string &logicalRoot) override;
};

class HeaderWriter
{
    std::unique_ptr<HeaderWriter_Interface> writer; // 智能指针，支持运行时切换模式

public:
    HeaderWriter() : writer(std::make_unique<HeaderWriter_v0>()) {} // 默认使用 v0 版本
    void headerWriter(std::vector<std::string> &filePathToScan, std::string &outPutFilePath, const std::string &logicalRoot);
    // 支持指定版本
    explicit HeaderWriter(std::unique_ptr<HeaderWriter_Interface> impl)
        : writer(std::move(impl)) {}

    // 封装后的调用方法
    void writeHeader(std::ofstream &outFile, fs::path &fullOutPath)
    {
        writer->writeHeader(outFile, fullOutPath);
    }
    void writeDirectory(std::ofstream &outFile, const std::vector<std::string> &filePathToScan, const fs::path &fullOutPath, const std::string &logicalRoot)
    {
        writer->writeDirectory(outFile, filePathToScan, fullOutPath, logicalRoot);
    };
};
