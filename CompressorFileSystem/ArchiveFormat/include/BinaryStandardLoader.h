#pragma once

#include "FileLibrary.h"
#include "ToolClasses.h"
#include "EntryDetails.h"
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
   encryptHeaderBlock(): 加密并回填目录块
*/
namespace Y_flib
{
  class BinaryStandardLoader
  {
  private:
    bool isReadHeader = false;
    bool blockIsDone = false;
    bool allDone = false;   // 标记是否完成所有目录读取
    bool firstReady = true; // 标记当前是否是目录就绪队列第一个元素

    Y_flib::FileCount countOfChildDirectory = 0; // 当前处理中或退出时目录下子目录或文件数量
    Y_flib::FileSize offset = 0;                 // 当前剩余字节数
    Y_flib::DirectoryOffsetSize tempOffset = 0;  // 当前处理块的大小（偏移）

    std::filesystem::path loadPath;
    std::filesystem::path parentPath;
    std::ifstream inFile;
    std::fstream fstreamForRefill;
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
    EntryQueue fileQueue;                                                  // 文件队列
    EntryQueue entryQueue;                                                 // 目录队列
    std::vector<std::array<Y_flib::DirectoryOffsetSize, 2>> blockPosition; // 目录数据块位置数组 1 为起点，2为大小

    // 解压时队列
    std::queue<std::filesystem::path> directoryQueueReady; // 目录恢复就绪队列，文件复原需要在目录恢复后操作

    BinaryStandardLoader() {};
    BinaryStandardLoader(const std::string inPath, std::vector<std::string> filePathToScan, std::filesystem::path parentPath)
    {
      this->loadPath = EncodingUtils::pathFromUtf8(inPath);

      this->inFile = std::ifstream(loadPath, std::ios::binary);

      this->fstreamForRefill = std::fstream(loadPath, std::ios::binary | std::ios::in | std::ios::out);

      if (!inFile)
        throw std::runtime_error("BinaryStandardLoader()-Error:Failed to open inFile" + inPath);
      // fstreamForRefill 用于压缩时回填加密目录块，解压时不需要写权限，允许打开失败

      this->filePathToScan = filePathToScan;
      this->parserForLoader = std::make_unique<EntryParser>(buffer, entryQueue, fileQueue, header, offset, tempOffset, this->filePathToScan);
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
      if (fstreamForRefill.is_open())
      {
        fstreamForRefill.close();
      }
    }

    Y_flib::DirectoryOffsetSize getDirectoryOffset() { return header.directoryOffset; } // 获取目录块偏移量

    Y_flib::CompressStrategy getStrategy() const { return header.strategy; } // 获取文件头中的策略号

    bool allLoopIsDone() { return allDone; } // 检查所有读取是否完成

    bool loaderRequestIsDone() { return blockIsDone; } // 检查当前块读取是否完成

    std::ifstream &getInFile() { return inFile; } // 获取输入文件对象

    void restartLoader(); // 重新打开文件并定位到当前偏移

    void encryptHeaderBlock(Y_flib::IEncryption &encryption, Y_flib::CompressionMode mode); // 在压缩流程中读取完目录信息就直接加密并回填目录块到文件
  };
  /*
   TODO(BinaryStandardLoader，小改动重构建议)

   这些建议刻意保持为“小步、低风险”修改，适合作为后续学习和整理时的
   参考清单。目标是改善封装，而不是一次性重写整个压缩/解压流程，也不改
   当前文件格式。

   1. 把内部队列收口到成员函数后面
      - 将 fileQueue / entryQueue / directoryQueueReady 改为 private。
      - 对外提供一个小接口，例如 loadNextBatch() 或 next()。
      - 目标：调用方不需要理解 loader 内部的队列协作规则。

   2. 用一个显式状态枚举替代零散的 bool 标记
      - 现在的 isReadHeader / blockIsDone / allDone 能工作，但状态组合是隐式的。
      - 之后可以考虑引入：
        enum class LoaderState { Created, HeaderLoaded, ReadingBlock,
        EmittingItems, Finished, Error };
      - 目标：让状态流转更容易理解、调试和排错。

   3. 让 restartLoader() 变成内部细节
      - 现在调用方必须知道什么时候该调 restartLoader()，
        什么时候该再次调用 headerLoaderIterator()。
      - 之后可以把这部分控制逻辑收回到 loader 内部。
      - 目标：减少外层循环对内部协议的感知。

   4. 不要让 fileQueue 的 pair.second 同时承担两种语义
      - 在压缩流程里，它更像是与 offset 相关的值。
      - 在解压流程里，它表示 compressedSize。
      - 之后可以改成显式任务结构体，让字段含义一眼就清楚。

   5. 尽量不要直接暴露原始 ifstream
      - getInFile() 会让外部代码直接改变 loader 的文件位置状态。
      - 之后更适合只暴露调用方真正需要的东西，例如 offset
        或一个专门的读取辅助接口。
      - 目标：把文件位置相关不变量尽量留在 loader 内部维护。

   6. 把“读取”和“回写”职责拆开
      - encryptHeaderBlock() 很有用，但它把“读取目录清单”和
        “回写目录块”两件事放在了同一个类里。
      - 之后可以考虑拆出一个辅助类，例如
        DirectoryBlockSealer / DirectoryBlockRewriter。
      - 目标：让 BinaryStandardLoader 更专注于读取和解析。

   如果以后要按顺序慢慢改，可以考虑：
     第一步：先隐藏公开队列
     第二步：加入 LoaderState
     第三步：引入显式任务结构体
     第四步：把 restart 逻辑内聚进去
     第五步：再拆出回写职责
  */
} // namespace Y_flib
