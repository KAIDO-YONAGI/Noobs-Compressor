#include "../include/Heffman.h"
#include <stdexcept>
#include <iostream>

//TODO: 检查方法中in_block使用完是否清空

Heffman::Heffman(int thread_nums):
    treeroot(NULL)
    {

    }

Heffman::~Heffman() {
    destroy_tree(treeroot);
}

void Heffman::statistic_freq(const int thread_id, const sfc::block_t& in_block)
{
    // 确保thread_tabs足够大
    if (thread_id >= (int)thread_tabs.size()) {
        thread_tabs.resize(thread_id + 1);
    }

    Heffmap &threadTab = thread_tabs[thread_id];
    for(auto& c: in_block)
    {
        threadTab[c].freq++;
    }
}

void Heffman::merge_ttabs(){
    hashtab.clear(); //清空旧的表
    auto iter_ttabs = thread_tabs.cbegin();
    auto ttabsend = thread_tabs.cend();
    while (iter_ttabs != ttabsend)
    {
        auto iter = iter_ttabs->cbegin();
        auto ttabend = iter_ttabs->cend();
        while (iter != ttabend)
        {
            hashtab[iter->first].add(iter->second);
            ++iter;  // 必须递增迭代器，否则会无限循环
        }
        ++iter_ttabs;  // 递增外层迭代器
    }
    //清空线程表
    for(auto& tab : thread_tabs)
    {
        tab.clear();
    }
}

std::unique_ptr<Minheap> Heffman::gen_minheap(){
    auto heap = std::make_unique<Minheap>();
    for(auto map : hashtab){
        Hefftreenode *node = new Hefftreenode(map.first, map.second.freq, true);
        heap->push(node);
    }
    return heap;
} 

void Heffman::gen_hefftree(){
    //清空旧的树
    if (treeroot != nullptr) {
        destroy_tree(treeroot);
        treeroot = nullptr;
    }
    auto heap = gen_minheap();
    while (heap->size() != 1)
    {
        Hefftreenode* left = heap->top();
        heap->pop();
        Hefftreenode* right = heap->top();
        heap->pop();
        Hefftreenode* parnt = new Hefftreenode('\0', left->freq+right->freq, left, right);
        heap->push(parnt);
    }
    treeroot = heap->top();
}

void Heffman::save_code_inTab(){
    // 重置pathStack
    pathStack.codeblocks.clear();
    pathStack.codelen = 0;

    run_save_code_inTab(treeroot);
}

void Heffman::run_save_code_inTab(Hefftreenode* root){
    if(root == NULL) return;

    if(root->isleaf == true){
        pathStack.writecode(hashtab[root->data]);
        return;  // 直接返回，不需要pop，因为调用者会pop
    }
    pathStack.push(0);
    run_save_code_inTab(root->left);
    pathStack.pop();
    pathStack.push(1);
    run_save_code_inTab(root->right);
    pathStack.pop();
}

void Heffman::encode(const sfc::block_t& in_block, sfc::block_t& out_block, BitHandler bitoutput){

    for(auto& c: in_block){
        bitoutput.handle(hashtab[c].code, hashtab[c].codelen, out_block);
    }

    // 处理最后不足8位的字节
    bitoutput.handle_last();
    if(bitoutput.bitlen > 0) {
        out_block.push_back(bitoutput.byte);
    }
}

void Heffman::findchar(Hefftreenode* &now, unsigned char& result, uint8_t toward){
    if(toward == 0){
        now = now->left;
    } else {
        now = now->right;
    }
    if(now != NULL && now->isleaf == true){
        result = now->data;
        now = treeroot;
    }
}

void Heffman::decode(const sfc::block_t& in_block, sfc::block_t& out_block, BitHandler bitinput, size_t maxOutputSize){
    Hefftreenode *now = treeroot;
    std::vector<uint8_t> treepath;  // 不预分配元素,只在需要时push_back
    treepath.reserve(8);  // 预留容量避免重新分配
    unsigned char result = 0;

    // 预留足够空间避免频繁重新分配
    out_block.reserve(in_block.size() * 2);

    for(auto& c: in_block)
    {
        bitinput.handle(c, treepath);
        for(auto toward: treepath)
        {
            if(now == NULL) break;

            // 先保存当前节点,因为findchar会修改now
            Hefftreenode* beforeMove = now;
            findchar(now, result, toward);

            // 如果移动前的节点在移动到叶子后now被重置了,说明找到了字符
            if(beforeMove != treeroot && now == treeroot){
                // 在push之前检查是否已达到maxOutputSize
                if(out_block.size() >= maxOutputSize) {
                    return;
                }
                out_block.push_back(result);
            }
        }
        treepath.clear();
    }
}

Hefftreenode* Heffman::getTreeRoot()
{
    return treeroot;
}

void Heffman::receiveTreRroot(Hefftreenode* root)
{
    treeroot = root;
}

void Heffman::destroy_tree(Hefftreenode* node) {
    if(node == NULL) return;
    destroy_tree(node->left);
    destroy_tree(node->right);
    delete node;
}

//序列化编码树并输出
void Heffman::tree_to_plat_uchar(sfc::block_t& out_block)
{
    std::stack<Hefftreenode*> stack;
    auto root = treeroot;
    stack.push(root);
    out_block.push_back('F');
    while(stack.empty() == false)
    {
        auto cur = stack.top();
        stack.pop();
        if(cur->isleaf == false)
        {
            out_block.push_back('r');
            if(cur->right==NULL || cur->left==NULL)
            {
                //TODO: 树错误
            }
            stack.push(cur->right);
            stack.push(cur->left);
        }
        else
            out_block.push_back('l');
        out_block.push_back(cur->data);    
    }
}

//解析编码表并加载树
void Heffman::spawn_tree(sfc::block_t& in_block)
{
    // 清空旧的树,避免内存泄漏和状态污染
    if (treeroot != nullptr) {
        std::cout << "[spawn_tree] Destroying old tree before spawning new one\n";
        destroy_tree(treeroot);
        treeroot = nullptr;
    } else {
        std::cout << "[spawn_tree] No old tree to destroy\n";
    }

    std::stack<Hefftreenode*> stack;

    auto iter_ib = in_block.cbegin();
    if(*iter_ib != 'F')
    {
        throw std::runtime_error("spawn_tree: Invalid tree format - missing 'F' header");
    }
    ++iter_ib;

    Hefftreenode* lastNode = nullptr;  // 记录最后处理的节点

    while(iter_ib != in_block.cend())
    {
        Hefftreenode *node = NULL;
        if(iter_ib + 1 == in_block.cend())
        {
            throw std::runtime_error("spawn_tree: Incomplete data - missing node data");
        }
        if(*iter_ib == 'r')
        {
            node = new Hefftreenode(*++iter_ib, 0, false);
            stack.push(node);
        }
        else if(*iter_ib == 'l')
        {
            node = new Hefftreenode(*++iter_ib, 0, true);
            // 叶子节点需要连接到栈顶的父节点
            while(!stack.empty())
            {
                Hefftreenode* parent = stack.top();
                bool parentComplete = connectNode(parent, node);

                if(parentComplete)
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
        if(node == NULL)
        {
            throw std::runtime_error("spawn_tree: Failed to create node");
        }
        lastNode = node;  // 记录最后的节点
        ++iter_ib;
    }

    // 如果栈空了,说明整棵树已经构建完成,根节点在lastNode中
    if(stack.empty())
    {
        treeroot = lastNode;
    }
    else if(stack.size() == 1)
    {
        treeroot = stack.top();
    }
    else
    {
        throw std::runtime_error("spawn_tree: Invalid tree structure - stack size is " +
                                std::to_string(stack.size()) + ", expected 0 or 1");
    }
}

bool Heffman::connectNode(Hefftreenode* p, Hefftreenode* c)
{
    if(p == NULL || c == NULL)
    {
        //TODO: 空指针异常
    }
    if(p->left == NULL)
    {
        p->left = c;
        return false;  // 左子节点连接,但父节点还没完成,不应该弹出
    }
    if(p->right == NULL)
    {
        p->right = c;
        return true;  // 右子节点也连接了,父节点完成,应该弹出
    }
    return false;  // 父节点已经有两个子节点,不能连接
}

// 调试方法：打印编码表统计信息
void Heffman::debugPrintCodeStats() {
    std::cout << "=== Huffman Code Table Stats ===\n";
    std::cout << "Total characters in table: " << hashtab.size() << "\n";

    int total_bits = 0;
    int max_codelen = 0;
    int min_codelen = 999;

    for(const auto& pair : hashtab) {
        const Chardata& cd = pair.second;
        if(cd.codelen > max_codelen) max_codelen = cd.codelen;
        if(cd.codelen < min_codelen) min_codelen = cd.codelen;
        total_bits += cd.codelen;

        // 打印前5个字符的编码详情
        static int count = 0;
        if(count < 5) {
            std::cout << "  Char '" << (char)pair.first << "' (0x" << std::hex << (int)pair.first << std::dec
                      << "): codelen=" << (int)cd.codelen
                      << ", code_bytes=" << cd.code.size() << "\n";
            count++;
        }
    }

    std::cout << "Average code length: " << (hashtab.size() > 0 ? (double)total_bits / hashtab.size() : 0) << " bits\n";
    std::cout << "Min code length: " << min_codelen << " bits\n";
    std::cout << "Max code length: " << max_codelen << " bits\n";
    std::cout << "================================\n";
}

