#include "../include/Heffman.h"
#include <stdexcept>

//TODO: 检查方法中in_block使用完是否清空

Heffman::Heffman(int thread_nums):
    treeroot(NULL) 
    { 
        
    }

void Heffman::statistic_freq(const int& thread_id, sfc::block_t& in_block)
{    
    try {
        Heffmap &threadTab = thread_tabs.at(thread_id);
        for(auto& c: in_block)
        {
            threadTab[c].freq++;
        }
    } catch (std::out_of_range) {

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
        }
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

void Heffman::encode(const int& thread_id, sfc::block_t& in_block, sfc::block_t& out_block, BitHandler bitoutput = BitHandler()){
    
    for(auto& c: in_block){
        bitoutput.handle(hashtab[c].code, hashtab[c].codelen, out_block);
    }
}

void Heffman::findchar(Hefftreenode* now, unsigned char* result, uint8_t toward){
    if(toward == 0){
        now = now->left;
    } else {
        now = now->right;
    }
    if(now->isleaf == true){
        *result = now->data;
        now = treeroot;
    } else {
        result = NULL;
    }
}

void Heffman::decode(const int& thread_id, sfc::block_t& in_block, sfc::block_t& out_block, BitHandler bitinput = BitHandler()){
    Hefftreenode *now = treeroot;
    std::vector<uint8_t> treepath(8);
    unsigned char *result = new unsigned char(NULL); 
    for(auto& c: in_block)
    {
        bitinput.handle(c, treepath);
        for(auto toward: treepath)
        {
            findchar(now, result, toward);
            if(result != NULL){
                out_block.push_back(*result);
            }
        }
        treepath.clear();
    }
    delete result;
}