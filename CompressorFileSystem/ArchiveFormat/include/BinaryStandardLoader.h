#pragma once

#include "FileLibrary.h"
#include "ToolClasses.h"
#include "EntryDetails.h"
#include "DirectoryCursor.h"
#include "EntryParser.h"
#include "IEncryption.h"
#include <queue>
#include <algorithm>
#include <cstring>
/*
 BinaryStandardLoader - 二进制目录块读取与解析器

   功能:
   从.sy文件读取加密的目录块
   调用DEntryarser解析二进制目录结构
   管理文件队列、目录队列用于解压流程
   支持分块读取和AES解密

   公共接口:
   headerLoaderIterator(): 主循环函数，逐块读取目录数据
   getDirectoryOffset(): 获取目录块偏移量
   allLoopIsDone(): 检查是否完成所有读取
   restartLoader(): 重新初始化读取状态
   takeBlockSpans(): 移交目录块位置记录（供 CatalogFinalizer 收尾加密）

   归档输出文件的写入分三个阶段依次进行，任何阶段都不并发写入（本类为纯读侧，不承担写入）：
   ① HeaderWriter（ofstream，建档阶段：写文件头、目录树、各预留字段）
   ② DataExporter（fstream，数据阶段：逐块纯追加）
   ③ CatalogFinalizer（fstream，收尾阶段：回填"处理后大小"槽 + 目录区原地加密）
   并行化时②③归专职写线程，见《线程池调研与改造计划.md》§7.1。
*/
namespace Y_flib
{
  class BinaryStandardLoader
  {
  public:
    /* 读取侧目录区游标（物理定义在 DirectoryCursor.h，独立头避免与 EntryParser 循环包含） */
    using ReadCursor = DirectoryReadCursor;

    // BlockSpan（目录块位置记录）已上移到 FileLibrary.h 公共类型区

  private:
    bool isReadHeader = false;
    bool blockIsDone = false;
    bool allDone = false;   // 标记是否完成所有目录读取
    bool firstReady = true; // 标记当前是否是目录就绪队列第一个元素

    Y_flib::FileCount countOfChildDirectory = 0; // 当前处理中或退出时目录下子目录或文件数量
    ReadCursor cursor;                           // 目录区读取游标（剩余量 + 当前块长）

    std::filesystem::path loadPath;
    std::filesystem::path parentPath;
    std::ifstream inFile;
    std::vector<std::string> filePathToScan; // 构造时初始化，而且只使用一次

    Y_flib::Header header;                        // 私有化存储当前文件头信息
    std::unique_ptr<EntryParser> parserForLoader; // 私有化工具类实例，避免重复构造与析构
    Y_flib::DataBlock buffer =
        Y_flib::DataBlock(Y_flib::Constants::BUFFER_SIZE + 1024); // 私有buffer,预留1024字节防止溢出

    void setRequestDone();                                                                                                            // 标记块读取完成
    void setAllLoopDone();                                                                                                            // 标记所有循环完成并清理资源
    void loadEntryBlock(StandardsReader &standardsReader, Y_flib::FileCount &countOfChildDirectory, Y_flib::IEncryption &encryption); // 读取单个数据块、解密、解析
    void loadHeaderStandard(std::ifstream &inFile, Y_flib::Header &header, Y_flib::DataBlock &buffer);
    void loadSeparatedStandard(Y_flib::FlagType &flag, StandardsReader &standardsReader, Y_flib::IvSize &ivNum);

  public:
    void headerLoaderIterator(Y_flib::IEncryption &encryption); // 主读取循环：逐块读取、解密、解析目录结构

    // 压缩时队列
    FileTaskQueue fileQueue;               // 文件任务队列（载荷语义见 FileTask）
    EntryQueue entryQueue;                 // 目录队列
    LinkTaskQueue linkQueueReady;           // 链接任务跨目录块累积，解压结束后统一创建
    std::vector<BlockSpan> blockPosition;  // 目录数据块位置记录，供收尾加密回写

    // 解压时队列
    std::queue<std::filesystem::path> directoryQueueReady; // 目录恢复就绪队列，文件复原需要在目录恢复后操作

    BinaryStandardLoader() {};
    BinaryStandardLoader(const std::string inPath, std::vector<std::string> filePathToScan, std::filesystem::path parentPath)
    {
      this->loadPath = EncodingUtils::pathFromUtf8(inPath);

      this->inFile = std::ifstream(loadPath, std::ios::binary);

      if (!inFile)
        throw std::runtime_error("BinaryStandardLoader()-Error:Failed to open inFile" + inPath);

      this->filePathToScan = filePathToScan;
      this->parserForLoader = std::make_unique<EntryParser>(
          buffer, entryQueue, fileQueue, linkQueueReady,
          header, cursor, this->filePathToScan);
      this->parentPath = parentPath;
    }

    ~BinaryStandardLoader()
    {
      setAllLoopDone();
      // 析构时关闭文件流
      if (inFile.is_open())
      {
        inFile.close();
      }
    }

    Y_flib::DirectoryOffsetSize getDirectoryOffset() { return header.directoryOffset; } // 获取目录块偏移量

    bool allLoopIsDone() { return allDone; } // 检查所有读取是否完成

    bool loaderRequestIsDone() { return blockIsDone; } // 检查当前块读取是否完成

    std::ifstream &getInFile() { return inFile; } // 获取输入文件对象

    void restartLoader(); // 重新打开文件并定位到当前偏移

    /* 移交目录块位置记录（take 后本类不再持有，供 CatalogFinalizer 收尾加密回写） */
    std::vector<BlockSpan> takeBlockSpans() { return std::move(blockPosition); }
  };
} // namespace Y_flib
