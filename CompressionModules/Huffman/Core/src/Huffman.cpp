#include "../include/Huffman.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <iostream>

// 输入块的所有权在调用方：本类只通过 const 引用读取 inBlock，
// 不负责清空或归还；清空/归还（如 BufferPool::release）由持有方处理。

Huffman::Huffman() : treeRoot(NULL), maxCodeLen(0)
{
    std::memset(blockFreq, 0, sizeof(blockFreq));
    std::memset(totalFreq, 0, sizeof(totalFreq));
    std::memset(codeLenTab, 0, sizeof(codeLenTab));
    std::memset(lut, 0, sizeof(lut));
}

Huffman::~Huffman()
{
    destroyTree(treeRoot);
}

void Huffman::statisticFreq(const sfc::block_t &inBlock)
{
    for (auto &c : inBlock)
    {
        ++blockFreq[c];
    }
}

void Huffman::finishFreqStat()
{
    // 原实现用「暂存表 → 总表」两段式，是为了让 genMinheap 的建堆顺序不受
    // unordered_map 迭代顺序影响（详见 genMinheap 注释）。
    // 改平坦数组后按符号值遍历天然有序，此处只需把暂存表原样搬到总表再清空暂存，
    // 语义与旧实现（hashTab.clear() 后逐个 add）完全一致。
    std::memcpy(totalFreq, blockFreq, sizeof(totalFreq));
    std::memset(blockFreq, 0, sizeof(blockFreq));
}

std::unique_ptr<Minheap> Huffman::genMinheap()
{
    // 按符号值 0..255 升序遍历入堆：与旧实现「先收集再按 (byte, freq) 排序」的
    // 入堆顺序完全一致，因此树结构与压缩输出字节保持不变；
    // 同时天然不依赖任何哈希容器的迭代顺序。
    auto heap = std::make_unique<Minheap>();
    for (int sym = 0; sym < 256; ++sym)
    {
        if (totalFreq[sym] > 0)
        {
            HuffTreeNode *node = new HuffTreeNode(static_cast<unsigned char>(sym), totalFreq[sym], true);
            heap->push(node);
        }
    }
    return heap;
}

void Huffman::genHufftree()
{
    // 清空旧的树
    if (treeRoot != nullptr)
    {
        destroyTree(treeRoot);
        treeRoot = nullptr;
    }

    auto heap = genMinheap();
    while (heap->size() != 1)
    {
        HuffTreeNode *left = heap->top();
        heap->pop();
        HuffTreeNode *right = heap->top();
        heap->pop();
        HuffTreeNode *parnt = new HuffTreeNode('\0', left->freq + right->freq, left, right);
        heap->push(parnt);
    }
    treeRoot = heap->top();
}

void Huffman::saveCodeInTab()
{
    // 重置pathStack
    pathStack.codeBlocks.clear();
    pathStack.codeLen = 0;
    // 重置符号编码表（len[] 归零即可；packed[] 的陈旧内容不会被读到）
    codeTab.clear();

    // 处理特殊情况：只有一个字符时，树的根节点本身就是叶子节点
    if (treeRoot != nullptr && treeRoot->isLeaf == true)
    {
        // 为这个唯一的字符分配编码 "0"
        pathStack.codeBlocks.clear();
        pathStack.codeBlocks.push_back(0);
        pathStack.codeLen = 1;
        pathStack.writeCode(codeTab, treeRoot->data);
        pathStack.codeBlocks.clear();
        pathStack.codeLen = 0;
    }
    else
    {
        runSaveCodeInTab(treeRoot);
    }

    // 树只用于导出码长；码字改按 canonical 规则统一重排，
    // 使解压侧能由码长表重建出完全相同的码字并用查表解码。
    buildCanonical();
}

void Huffman::runSaveCodeInTab(HuffTreeNode *root)
{
    if (root == NULL)
        return;

    if (root->isLeaf == true)
    {
        pathStack.writeCode(codeTab, root->data);
        return; // 直接返回，不需要pop，因为调用者会pop
    }
    pathStack.push(0);
    runSaveCodeInTab(root->left);
    pathStack.pop();
    pathStack.push(1);
    runSaveCodeInTab(root->right);
    pathStack.pop();
}

// 由 codeTab.len（树导出的码长）构造 canonical 码。
// canonical 规则：码字按 (码长, 符号值) 升序连续分配，
//     firstCode[L+1] = (firstCode[L] + count[L]) << 1
// 这样解压侧只需码长表就能复现同一套码字，无需整棵树。
void Huffman::buildCanonical()
{
    for (int L = 0; L <= kMaxCodeLen; ++L)
    {
        canonCount[L] = 0;
        canonFirstCode[L] = 0;
        canonFirstIdx[L] = 0;
    }
    maxCodeLen = 0;

    for (int s = 0; s < 256; ++s)
    {
        codeLenTab[s] = codeTab.len[s];
        if (codeLenTab[s] != 0 && codeLenTab[s] > maxCodeLen)
        {
            maxCodeLen = codeLenTab[s];
        }
    }

    // 256 符号 + 8MB 块（8.4M 样本）下，Huffman 深度受 Fibonacci 界约束：
    // F(35)≈9.2e6 > 8.4e6，故 maxCodeLen ≤ 32，绝无可能超过 64。
    // 仍保留防御性校验，避免任何越界/溢出变成静默的数据损坏。
    if (maxCodeLen > 64)
    {
        throw std::runtime_error("Huffman: code length exceeds 64 bits");
    }
    if (maxCodeLen == 0)
    {
        return;
    }

    for (int s = 0; s < 256; ++s)
    {
        if (codeLenTab[s] != 0)
        {
            ++canonCount[codeLenTab[s]];
        }
    }

    // 按 (码长, 符号值) 升序排列
    int cursor = 0;
    for (int L = 1; L <= maxCodeLen; ++L)
    {
        for (int s = 0; s < 256; ++s)
        {
            if (codeLenTab[s] == L)
            {
                sortedSymbols[cursor++] = static_cast<uint8_t>(s);
            }
        }
    }

    // 各码长的首码字与首下标
    uint64_t code = 0;
    cursor = 0;
    for (int L = 1; L <= maxCodeLen; ++L)
    {
        canonFirstCode[L] = code;
        canonFirstIdx[L] = static_cast<uint16_t>(cursor);
        cursor += canonCount[L];
        code = (code + canonCount[L]) << 1;
    }

    // 回写码字供 encode 使用：code[] 为右对齐 uint64（快路径）；
    // packed[] 仅在码长 > 64 时需要，故只在此时填充，避免每块 256 次小分配。
    for (int L = 1; L <= maxCodeLen; ++L)
    {
        const uint64_t first = canonFirstCode[L];
        const uint16_t idx0 = canonFirstIdx[L];
        const uint16_t n = canonCount[L];
        for (uint16_t i = 0; i < n; ++i)
        {
            const uint8_t sym = sortedSymbols[idx0 + i];
            const uint64_t cw = first + i;
            codeTab.code[sym] = cw;
            if (L > 64)
            {
                CodeT &pk = codeTab.packed[sym];
                pk.assign(static_cast<size_t>((L + 7) / 8), 0);
                for (int b = 0; b < L; ++b)
                {
                    if ((cw >> (L - 1 - b)) & 1ULL)
                    {
                        pk[static_cast<size_t>(b) / 8] |= static_cast<uint8_t>(1u << (7 - (b % 8)));
                    }
                }
            }
        }
    }
}

// 由 canonical 码构造 12bit 主表。
// 表项 0 表示「该 12bit 前缀属于长码（>12bit）」，解码时走 canonical 逐位回退。
// 码长 ≤ 12 的符号把覆盖自己的全部 12bit 后缀一次填满。
void Huffman::buildDecodeLut()
{
    for (int i = 0; i < kLutSize; ++i)
    {
        lut[i] = 0;
    }
    if (maxCodeLen <= 0)
    {
        return;
    }

    const int fillLen = (maxCodeLen < kLutBits) ? maxCodeLen : kLutBits;
    for (int L = 1; L <= fillLen; ++L)
    {
        const uint32_t span = 1u << (kLutBits - L);
        const uint64_t first = canonFirstCode[L];
        const uint16_t idx0 = canonFirstIdx[L];
        const uint16_t n = canonCount[L];
        for (uint16_t i = 0; i < n; ++i)
        {
            const uint8_t sym = sortedSymbols[idx0 + i];
            const uint32_t base = static_cast<uint32_t>(first + i) << (kLutBits - L);
            const uint16_t entry = static_cast<uint16_t>((L << 8) | sym);
            for (uint32_t k = 0; k < span; ++k)
            {
                lut[base + k] = entry;
            }
        }
    }
}

void Huffman::encode(const sfc::block_t &inBlock, sfc::block_t &outBlock, BitHandler bitOutput)
{
    // 在数据块开始前写入填充位数标记（1字节）
    // 这个字节稍后会被更新为实际的填充位数
    size_t paddingBitsPos = outBlock.size();
    outBlock.push_back(0); // 占位符，稍后更新

    for (auto &c : inBlock)
    {
        const CodeLenT len = codeTab.len[c];
        // 检查字符是否在编码表中
        if (len == 0)
        {
            throw std::runtime_error("Character not in Huffman encoding table");
        }
        // 快速路径前置条件：暂存位 + 码长 <= 64（避免 64 位移位溢出）
        if (len <= 64 && static_cast<unsigned>(bitOutput.bitLen) + len <= 64)
        {
            bitOutput.handleFast(codeTab.code[c], len, outBlock);
        }
        else
        {
            // 罕见回退：码长 > 64，或暂存位 + 码长会超出 64 位
            bitOutput.handle(codeTab.packed[c], len, outBlock);
        }
    }

    // 处理最后不足8位的字节
    bitOutput.handleLast();
    uint8_t paddingBits = 0;
    if (bitOutput.bitLen > 0)
    {
        paddingBits = 8 - bitOutput.bitLen; // 计算填充的位数
        outBlock.push_back(bitOutput.byte);
    }

    // 更新填充位数标记
    outBlock[paddingBitsPos] = paddingBits;
}

// canonical 码 + 12bit 查表解码。
// 取代原实现：原实现把每个输入字节展开成 8 个元素的 vector（BitHandler::handle），
// 再逐位 findchar 走树；现在每符号只需一次 12bit 表查询。
void Huffman::decode(const sfc::block_t &inBlock, sfc::block_t &outBlock, BitHandler bitInput, size_t maxOutputSize)
{
    (void)bitInput; // 解码自维护 64 位位缓冲，不再经 BitHandler 逐字节展开

    if (inBlock.size() < 1)
    {
        throw std::runtime_error("decode: input block too small (missing padding bits marker)");
    }

    // 读取填充位数标记（第一个字节）
    const uint8_t paddingBits = inBlock[0];
    if (paddingBits > 7)
    {
        throw std::runtime_error("decode: invalid padding bits value: " + std::to_string(paddingBits));
    }

    // 有效比特总数：首字节为填充标记；最后一个数据字节的低 paddingBits 位为填充
    size_t totalBits = (inBlock.size() - 1) * 8;
    if (paddingBits > 0 && inBlock.size() > 1)
    {
        totalBits -= paddingBits;
    }

    // 预留足够空间避免频繁重新分配
    outBlock.reserve(inBlock.size() * 2);

    if (totalBits == 0)
    {
        return;
    }

    uint64_t acc = 0;    // 低位对齐的位缓冲
    int nbits = 0;       // acc 中的有效位数
    size_t ip = 1;       // 输入字节游标
    size_t consumed = 0; // 已消费比特数

    // 补齐位缓冲（每次最多补到 64 位；最后一个字节按 paddingBits 截取有效高位）
    auto refill = [&]() {
        while (nbits <= 56 && ip < inBlock.size())
        {
            const bool isLast = (ip + 1 == inBlock.size());
            int valid = 8;
            if (isLast && paddingBits > 0)
            {
                valid = 8 - paddingBits; // 编码侧把有效位左移到高位
            }
            const uint8_t byte = inBlock[ip++];
            if (valid <= 0)
            {
                break;
            }
            acc = (acc << valid) | static_cast<uint64_t>(byte >> (8 - valid));
            nbits += valid;
        }
    };

    // 丢弃已消费的高位
    auto drop = [&](int n) {
        nbits -= n;
        acc &= (nbits >= 64) ? ~0ULL : ((1ULL << nbits) - 1ULL);
    };

    while (consumed < totalBits && outBlock.size() < maxOutputSize)
    {
        refill();
        if (nbits == 0)
        {
            break;
        }

        // 窥视高 12 位（不足则左移到 12 位宽度，低位补零）
        const int use = (nbits < kLutBits) ? nbits : kLutBits;
        const uint32_t idx = static_cast<uint32_t>((acc >> (nbits - use)) << (kLutBits - use));
        const uint16_t entry = lut[idx];

        if (entry != 0)
        {
            const int len = entry >> 8;
            if (len > nbits || static_cast<size_t>(len) > totalBits - consumed)
            {
                break; // 流不一致，保护性退出
            }
            outBlock.push_back(static_cast<uint8_t>(entry & 0xFFu));
            drop(len);
            consumed += static_cast<size_t>(len);
            continue;
        }

        // 长码回退（码长 > 12bit）：canonical 逐位解码，不消费已窥视的位
        uint64_t code = 0;
        bool found = false;
        for (int L = 1; L <= maxCodeLen && consumed < totalBits; ++L)
        {
            if (nbits == 0)
            {
                refill();
            }
            if (nbits == 0)
            {
                break;
            }
            code = (code << 1) | ((acc >> (nbits - 1)) & 1ULL);
            drop(1);
            ++consumed;

            // code < firstCode 时相减会下溢成极大值，自然不满足 < count，无需额外判断
            if ((code - canonFirstCode[L]) < static_cast<uint64_t>(canonCount[L]))
            {
                if (outBlock.size() < maxOutputSize)
                {
                    outBlock.push_back(sortedSymbols[canonFirstIdx[L] +
                                                     static_cast<uint32_t>(code - canonFirstCode[L])]);
                }
                found = true;
                break;
            }
        }
        if (!found)
        {
            break; // 到达末尾或流损坏
        }
    }
}

void Huffman::destroyTree(HuffTreeNode *node)
{
    if (node == NULL)
        return;
    destroyTree(node->left);
    destroyTree(node->right);
    delete node;
}

// 序列化码长表：'L' + 最大码长(1B) + 256 字节码长（0 = 该符号未出现），共 258 字节。
// 取代原「2 字节/节点」的整树序列化（满字母表约 1023 字节）。
void Huffman::codeTableToPlatUchar(sfc::block_t &outBlock)
{
    outBlock.push_back('L');
    outBlock.push_back(static_cast<uint8_t>(maxCodeLen));
    for (int s = 0; s < 256; ++s)
    {
        outBlock.push_back(codeLenTab[s]);
    }
}

// 由码长表重建 canonical 码与解码查找表（不再重建树）
void Huffman::spawnCodeTable(const sfc::block_t &inBlock)
{
    if (inBlock.size() != 258 || inBlock[0] != 'L')
    {
        throw std::runtime_error("spawnCodeTable: invalid code-length table");
    }

    const int declared = inBlock[1];
    if (declared <= 0 || declared > 64)
    {
        throw std::runtime_error("spawnCodeTable: invalid max code length");
    }

    codeTab.clear();
    for (int s = 0; s < 256; ++s)
    {
        codeTab.len[s] = inBlock[2 + s];
    }

    buildCanonical();
    if (maxCodeLen != declared)
    {
        throw std::runtime_error("spawnCodeTable: max code length mismatch");
    }
    buildDecodeLut();
}
