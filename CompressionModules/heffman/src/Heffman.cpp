#include "../include/Heffman.h"
#include <stdexcept>

//TODO: 检查方法中in_block使用完是否清空

Heffman::Heffman(int thread_nums):
    treeroot(NULL)
    {

    }

Heffman::~Heffman() {
    destroy_tree(treeroot);
}

void Heffman::statistic_freq(const int& thread_id, const sfc::block_t& in_block)
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
    run_save_code_inTab(treeroot);
}

void Heffman::run_save_code_inTab(Hefftreenode* root){
    if(root == NULL) return;

    if(root->isleaf == true){
        pathStack.writecode(hashtab[root->data]);
        pathStack.pop();
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

void Heffman::decode(const sfc::block_t& in_block, sfc::block_t& out_block, BitHandler bitinput){
    Hefftreenode *now = treeroot;
    std::vector<uint8_t> treepath(8);
    unsigned char result = 0;
    for(auto& c: in_block)
    {
        bitinput.handle(c, treepath);
        for(auto toward: treepath)
        {
            if(now == NULL) break;
            findchar(now, result, toward);
            if(now->isleaf == true){
                out_block.push_back(result);
                now = treeroot;
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
    std::stack<Hefftreenode*> stack;

    auto iter_ib = in_block.cbegin();
    if(*iter_ib != 'F')
    {
        //TODO: 解析编码表异常，验证错误
    }
    ++iter_ib;
    while(iter_ib != in_block.cend())
    {
        Hefftreenode *node = NULL;
        if(iter_ib + 1 == in_block.cend())
        {
            //TODO: 解析编码表异常，数据缺失
        }
        if(*iter_ib == 'r')
        {
            node = new Hefftreenode(*++iter_ib, 0, false);
            stack.push(node);
        }
        else if(*iter_ib == 'l')
        {
            node = new Hefftreenode(*++iter_ib, 0, true);
            while(!stack.empty() && connectNode(stack.top(), node))
            {
                stack.pop();
            }
        }
        if(node == NULL)
        {
            //TODO: 解析编码表异常，创建节点失败
        }
        ++iter_ib;
    }

    while(stack.size() != 1)
        stack.pop();
    treeroot = stack.top();
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
        return true;
    }
    if(p->right == NULL)
    {
        p->right = c;
        return true;
    }
    return false;
}
