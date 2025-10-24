#include "../include/Heffman.h"
#include <stdexcept>

Heffman::Heffman(int thread_nums):
    treeroot(NULL) 
    { 
        
    }

void Heffman::statistic_freq(int thread_id, sfc::blocks_t* data_blocks_in)
{    
    Heffmap threadTab;
    sfc::block_t *pblock;
    try {
        pblock = data_blocks_in->at(thread_id - 1);
        threadTab = thread_tabs.at(thread_id - 1);
    } catch (std::out_of_range) {

    }
    auto iter = pblock->cbegin();
    auto end = pblock->cend();
    while (iter != end)
    {
        threadTab[*iter++].add();
    }
    pblock->clear();
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

void Heffman::encode(int thread_id, BitHandler bitoutput = BitHandler()){
    auto pblock = data_blocks->at(thread_id - 1);
    auto outputblock = data_blocks_out->at(thread_id - 1);
    for(auto c: *pblock){
        bitoutput.handle(hashtab[c].code, hashtab[c].codelen, outputblock);
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

void Heffman::decode(int thread_id, BitHandler bitinput = BitHandler()){
    auto pblock = data_blocks->at(thread_id - 1);
    auto outputblock = data_blocks_out->at(thread_id - 1);
    Hefftreenode *now = treeroot;
    std::vector<uint8_t> treepath(8);
    unsigned char *result = new unsigned char(NULL); 
    for(auto c: *pblock)
    {
        bitinput.handle(c, treepath);
        for(auto toward: treepath)
        {
            findchar(now, result, toward);
            if(result != NULL){
                outputblock->push_back(*result);
            }
        }
        treepath.clear();
    }
    delete result;
}