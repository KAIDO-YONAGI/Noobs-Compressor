// DirectoryCursor.h
#pragma once

#include "FileLibrary.h"

namespace Y_flib
{
    /* DirectoryReadCursor - 目录区读取游标
     *
     * 集中维护目录区读取的两个状态（此前以 offset/tempOffset 两个裸成员
     * 散落在 BinaryStandardLoader 与 EntryParser 之间，且与写入侧同名不同义）：
     *   remaining —— 目录区剩余未读字节数（不含文件头，从数据区起点倒推）
     *   blockLen  —— 当前块长度；0 表示最后一块（末块长度不预置，由剩余量推出）
     *
     * 物理上定义为独立头 + BinaryStandardLoader 内 using 别名（ReadCursor）：
     * EntryParser 与 Loader 都要引用该类型，若嵌套定义在 Loader 类内
     * 会造成两个头文件循环包含。
     */
    struct DirectoryReadCursor
    {
        BlockLength remaining = 0;
        BlockLength blockLen = 0;

        /* 读入文件头后初始化：dataAreaStart 为 header.directoryOffset（数据区起点） */
        void initFromHeader(SlotOffset dataAreaStart)
        {
            remaining = dataAreaStart - Constants::HEADER_SIZE;
        }

        /* 下一个未读目录块的位置（数据区起点倒推剩余量；原三处重复推导收口于此） */
        SlotOffset nextReadPos(SlotOffset dataAreaStart) const
        {
            return dataAreaStart - remaining;
        }

        /* 目录区是否只剩结尾魔数（全部块已读完） */
        bool onlyMagicRemains() const { return remaining == sizeof(SizeOfMagicNum); }

        bool hasData() const { return remaining > 0; }

        bool isLastBlock() const { return blockLen == 0; }

        /* 本块应读取的字节数：末块（长度槽为 0）由剩余量去掉结尾魔数推出。
           注意调用时当前块已从 remaining 中扣减（见 consumeSeparated） */
        BlockLength bytesInThisBlock() const
        {
            return isLastBlock() ? remaining - sizeof(SizeOfMagicNum) : blockLen;
        }

        /* 读出一个分割标准后：剩余量跳过槽位与块体，并记下本块长度 */
        void consumeSeparated(BlockLength len)
        {
            remaining -= Constants::SEPARATED_STANDARD_SIZE + len;
            blockLen = len;
        }

        /* 末块读完后扣减剩余量（非末块已在 consumeSeparated 中扣过） */
        void consumeFinalBlock(BlockLength readSize) { remaining -= readSize; }

        /* 该文件条目"处理后大小"预留字段的绝对偏移（原 EntryParser 中最复杂的偏移公式）：
           目录区当前位置（dataAreaStart-remaining）回退一个块长到本块块首，
           再前进块内游标 bufferPtr 到条目内字段 */
        SlotOffset entryFieldPos(SlotOffset dataAreaStart, BlockLength bufferPtr) const
        {
            return dataAreaStart - (remaining + blockLen) + bufferPtr;
        }

        /* 块内游标是否已消费完本块 */
        bool blockEndReached(BlockLength bufferPtr) const
        {
            return blockLen != 0 && blockLen <= bufferPtr;
        }
    };
} // namespace Y_flib
