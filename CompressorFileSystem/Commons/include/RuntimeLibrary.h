// RuntimeLibrary.h
#pragma once
// 运行期通用类型（Y_flib::Runtime 子命名空间）
//
// 收录与归档磁盘格式无关、只存在于内存中的类型：
// 字节缓冲、缓冲区尺寸、运行期模式枚举。
// 归档磁盘格式类型（字段宽度别名、文件标准、布局结构、Constants）仍在 FileLibrary.h。
//
// 兼容性：FileLibrary.h 对本头文件的类型做了 using 再导出，
// 既有 Y_flib::DataBlock 等写法继续有效；新代码建议直接写 Y_flib::Runtime:: 全称。

#include <cstdint>
#include <vector>

namespace Y_flib::Runtime
{
    /* 字节缓冲区：压缩/加密接口与缓冲池的通用数据载体 */
    using DataBlock = std::vector<unsigned char>;

    /* 缓冲区尺寸类型（Constants::BUFFER_SIZE 等缓冲容量常量的类型） */
    using UpSizeOfBuffer = uint32_t;

    /* 压缩元数据标记字节（如 NullCompression 的空标记） */
    using MetadataMarker = uint8_t;

    // 压缩模式策略枚举（运行期配置；落盘的是 modeToId 映射出的 CompressStrategy 字节）
    enum class CompressionMode : uint8_t
    {
        HuffmanAES  = 0, // 默认：Huffman压缩 + AES加密（向后兼容）
        HuffmanOnly = 1, // 仅Huffman压缩，无加密
        AESOnly     = 2, // 仅AES加密，无压缩
        PackOnly    = 3  // 仅打包，无压缩无加密
    };

    enum class ParserMode // 运行时解析方式，不写入归档，无需固定底层宽度
    {
        Compression,  // 压缩：扫描本地文件
        Decompression // 解压：读取目录块
    };

    enum class AesMode // AES 处理方向，运行时状态，不写入归档，无需固定底层宽度
    {
        Encrypt, // 加密
        Decrypt  // 解密
    };
} // namespace Y_flib::Runtime
