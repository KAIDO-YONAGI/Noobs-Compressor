#pragma once

#include "FileLibrary.h"
#include "Directory_FileDetails.h"
#include "ToolClasses.h"

/* Directory_FileParser - 二进制目录结构解析器
 *
 * 功能:
 *   解析二进制目录块中的文件/目录信息
 *   支持压缩模式（扫描本地文件）和解压模式（读取加密块）
 *   使用模板化解析函数处理多种数据类型
 *   管理文件队列和目录队列用于流程控制
 *
 * 公共接口:
 *   parser(): 主解析函数，处理缓冲区中的目录数据
 *   setRootForDecompression(): 设置解压时的根路径
 */
class Directory_FileParser
{
private:
    Directory_FileQueue &directoryQueue;
    Directory_FileQueue &fileQueue;
    std::vector<std::string> &filePathToScan;
    DataBlock &buffer;
    Transfer transfer;
    const Header &header;
    const DirectoryOffsetSize_uint &offset;
    const DirectoryOffsetSize_uint &tempOffset;
    size_t parserMode = 0; // 0：默认模式、1：压缩模式、2：解压模式

    /* 检查缓冲区读取范围是否有效，防止越界 */
    void checkBounds(DirectoryOffsetSize_uint pos, FileNameSize_uint equiredSize) const;

    /* 模板化函数：解析文件名长度和内容，安全构造std::string */
    template <typename T>
    void fileName_fileSizeParser(
        T &fileNameSize,
        std::string &fileName,
        DirectoryOffsetSize_uint &bufferPtr)
    {
        try
        {
            // 1. 读取文件名长度
            checkBounds(bufferPtr, sizeof(T));
            memcpy(&fileNameSize, buffer.data() + bufferPtr, sizeof(T));
            bufferPtr += sizeof(T);

            // 2. 读取字符串内容,安全构造 std::string,防止未初始化的越界报错
            checkBounds(bufferPtr, static_cast<FileNameSize_uint>(fileNameSize));
            fileName.assign(
                buffer.data() + bufferPtr,
                buffer.data() + bufferPtr + fileNameSize);
            bufferPtr += fileNameSize;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(
                "fileName_fileSizeParser failed at offset " +
                std::to_string(bufferPtr) + ": " + e.what());
        }
    }

    /* 模板化函数：按指定类型解析数值，返回解析的值 */
    template <typename T>
    T numsParser(DirectoryOffsetSize_uint &bufferPtr)
    {
        try
        {
            T num;
            checkBounds(bufferPtr, sizeof(T));
            memcpy(&num, buffer.data() + bufferPtr, sizeof(T));
            bufferPtr += sizeof(T);

            return num;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(
                "numsParser failed at offset " +
                std::to_string(bufferPtr) + ": " + e.what());
        }
    }

    /* 连接文件名到当前路径，返回完整路径 */
    fs::path pathConnector(std::string &fileName);

    /* 解析单个文件的元数据，将文件入队 */
    void fileParser(DirectoryOffsetSize_uint &bufferPtr);

    /* 解析单个目录的元数据和子元素，将目录入队 */
    void directoryParser(DirectoryOffsetSize_uint &bufferPtr);

    /* 解析根目录节点，处理多路径扫描 */
    void rootParser(DirectoryOffsetSize_uint &bufferPtr,const std::vector<std::string> &filePathToScan,FileCount_uint &countOfKidDirectory,bool &noDirec);

public:
    std::string rootForDecompression;

    /* 设置解压时的根目录路径 */
    void setRootForDecompression(std::string rootForDecompression)
    {
        this->rootForDecompression = rootForDecompression;
    }

    /* 构造函数，初始化解析器，自动检测压缩/解压模式 */
    Directory_FileParser(DataBlock &buffer, Directory_FileQueue &directoryQueue,
                         Directory_FileQueue &fileQueue, const Header &header,
                         const DirectoryOffsetSize_uint &offset,
                         const DirectoryOffsetSize_uint &tempOffset,
                         std::vector<std::string> &filePathToScan)
        : buffer(buffer), directoryQueue(directoryQueue),
          fileQueue(fileQueue),
          header(header),
          offset(offset), tempOffset(tempOffset),filePathToScan(filePathToScan)
    {
        parserMode=((!filePathToScan.empty())?1:2);//非空表示压缩
    }

    /* 主解析函数，处理缓冲区中的目录数据块 */
    void parser(DirectoryOffsetSize_uint &bufferPtr, FileCount_uint &countOfKidDirectory);
};