#include "../include/Huffman.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <iostream>

// 输入块的所有权在调用方：本类只通过 const 引用读取 inBlock，
// 不负责清空或归还；清空/归还（如 BufferPool::release）由持有方处理。

Huffman::Huffman() : treeRoot(NULL)
{
    std::memset(blockFreq, 0, sizeof(blockFreq));
    std::memset(totalFreq, 0, sizeof(totalFreq));
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

bool Huffman::findchar(HuffTreeNode *&now, unsigned char &result, uint8_t toward)
{
    if (toward == 0)
    {
        now = now->left;
    }
    else
    {
        now = now->right;
    }
    if (now != NULL && now->isLeaf == true)
    {
        result = now->data;
        now = treeRoot;
        return true; // 找到了一个字符
    }
    return false; // 还在树的中间节点
}

void Huffman::decode(const sfc::block_t &inBlock, sfc::block_t &outBlock, BitHandler bitInput, size_t maxOutputSize)
{
    if (inBlock.size() < 1)
    {
        throw std::runtime_error("decode: input block too small (missing padding bits marker)");
    }

    // 读取填充位数标记（第一个字节）
    uint8_t paddingBits = inBlock[0];
    if (paddingBits > 7)
    {
        throw std::runtime_error("decode: invalid padding bits value: " + std::to_string(paddingBits));
    }

    HuffTreeNode *now = treeRoot;
    std::vector<uint8_t> treePath; // 不预分配元素,只在需要时push_back
    treePath.reserve(8);           // 预留容量避免重新分配
    unsigned char result = 0;
    size_t totalBitsProcessed = 0;
    size_t charsDecoded = 0;

    // 预留足够空间避免频繁重新分配
    outBlock.reserve(inBlock.size() * 2);

    // 计算最后有填充的字节位置（如果paddingBits>0，最后一个字节才有填充）
    // 如果paddingBits==0，使用SIZE_MAX表示没有字节有填充（避免误匹配索引0）
    size_t lastByteIdx = (paddingBits > 0) ? (inBlock.size() - 1) : SIZE_MAX;

    // 特殊处理：如果树只有一个叶子节点（根节点本身就是叶子），直接根据比特数量输出字符
    if (treeRoot != nullptr && treeRoot->isLeaf == true)
    {
        // 计算总比特数
        size_t totalBits = 0;
        for (size_t idx = 1; idx < inBlock.size(); ++idx)
        {
            size_t validBits = (idx == lastByteIdx && paddingBits > 0) ? (8 - paddingBits) : 8;
            totalBits += validBits;
        }

        // 对于单叶子树，每一比特代表一个字符
        for (size_t i = 0; i < totalBits && outBlock.size() < maxOutputSize; ++i)
        {
            outBlock.push_back(treeRoot->data);
        }
        return;
    }

    // 从第二个字节开始处理（跳过填充位数标记）
    for (size_t idx = 1; idx < inBlock.size(); ++idx)
    {
        unsigned char c = inBlock[idx];

        // 只有当这是最后有填充的字节时，才考虑填充位
        uint8_t validBits = (idx == lastByteIdx && paddingBits > 0) ? (8 - paddingBits) : 8;

        bitInput.handle(c, treePath, validBits);
        totalBitsProcessed += validBits;

        for (auto toward : treePath)
        {
            if (now == NULL)
            {
                break;
            }

            // 调用findchar并检查是否找到了字符
            bool foundChar = findchar(now, result, toward);

            // 如果找到了字符，输出它
            if (foundChar)
            {
                // 在push之前检查是否已达到maxOutputSize
                if (outBlock.size() >= maxOutputSize)
                {
                    return;
                }
                outBlock.push_back(result);
                charsDecoded++;
            }
            else if (now == NULL)
            {
                break;
            }
        }
        treePath.clear();
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

// 序列化编码树并输出
void Huffman::treeToPlatUchar(sfc::block_t &outBlock)
{
    std::stack<HuffTreeNode *> stack;
    auto root = treeRoot;
    stack.push(root);
    outBlock.push_back('F');
    while (stack.empty() == false)
    {
        auto cur = stack.top();
        stack.pop();
        if (cur->isLeaf == false)
        {
            outBlock.push_back('r');
            if (cur->right == NULL || cur->left == NULL)
            {
                throw std::runtime_error("treeToPlatUchar: 内部节点缺少子节点，编码树已损坏");
            }
            stack.push(cur->right);
            stack.push(cur->left);
        }
        else
            outBlock.push_back('l');
        outBlock.push_back(cur->data);
    }
}

// 解析编码表并加载树
void Huffman::spawnTree(sfc::block_t &inBlock)
{
    // 清空旧的树,避免内存泄漏和状态污染
    if (treeRoot != nullptr)
    {
        destroyTree(treeRoot);
        treeRoot = nullptr;
    }
    std::stack<HuffTreeNode *> stack;

    auto iter_ib = inBlock.cbegin();
    if (*iter_ib != 'F')
    {
        throw std::runtime_error("spawnTree: Invalid tree format - missing 'F' header");
    }
    ++iter_ib;

    HuffTreeNode *lastNode = nullptr; // 记录最后处理的节点

    while (iter_ib != inBlock.cend())
    {
        HuffTreeNode *node = NULL;
        if (iter_ib + 1 == inBlock.cend())
        {
            throw std::runtime_error("spawnTree: Incomplete data - missing node data");
        }
        if (*iter_ib == 'r')
        {
            node = new HuffTreeNode(*++iter_ib, 0, false);
            stack.push(node);
        }
        else if (*iter_ib == 'l')
        {
            node = new HuffTreeNode(*++iter_ib, 0, true);
            // 叶子节点需要连接到栈顶的父节点
            while (!stack.empty())
            {
                HuffTreeNode *parent = stack.top();
                bool parentComplete = connectNode(parent, node);

                if (parentComplete)
                {
                    // 父节点完成,弹出并作为新的子节点继续向上连接
                    stack.pop();
                    node = parent;
                }
                else
                {
                    // 父节点还没完成(只连接了左子节点),停止
                    break;
                }
            }
        }
        if (node == NULL)
        {
            throw std::runtime_error("spawnTree: Failed to create node");
        }
        lastNode = node; // 记录最后的节点
        ++iter_ib;
    }

    // 如果栈空了,说味著整棵树已经构建完成,根节点在lastNode中
    if (stack.empty())
    {
        treeRoot = lastNode;
    }
    else if (stack.size() == 1)
    {
        treeRoot = stack.top();
    }
    else
    {
        throw std::runtime_error("spawnTree: Invalid tree structure - stack size is " +
                                 std::to_string(stack.size()) + ", expected 0 or 1");
    }
}

bool Huffman::connectNode(HuffTreeNode *p, HuffTreeNode *c)
{
    if (p == NULL || c == NULL)
    {
        throw std::runtime_error("connectNode: 节点指针为空");
    }
    if (p->left == NULL)
    {
        p->left = c;
        return false; // 左子节点连接,但父节点还没完成,不应该弹出
    }
    if (p->right == NULL)
    {
        p->right = c;
        return true; // 右子节点也连接了,父节点完成,应该弹出
    }
    return false; // 父节点已经有两个子节点,不能连接
}
