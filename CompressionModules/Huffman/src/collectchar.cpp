#include "heffman.h"


int hash(unsigned char c){
    return c%61;
}

Charnode* newHashTab(){
    Charnode *tab = (Charnode*)malloc(CAPACITY*sizeof(Charnode));
    for(int i=0;i<CAPACITY;++i){
        tab[i].next = NULL;
    }
    return tab;
}

void insert_hashtab(Charnode* tab,unsigned char c){
    int index = hash(c);
    tab[index].next = insert_list(tab[index].next,c);
}


Charnode* insert_list(Charnode* list, unsigned char c) {
    Charnode *cur = list;
    
    // 检查字符是否已存在
    while(cur != NULL) {
        if(cur->c == c) {
            ++cur->num;
            return list;
        }
        cur = cur->next;
    }
    
    // 创建新节点
    Charnode *node = (Charnode*)malloc(sizeof(Charnode));
    if(node == NULL) {
        return list; // 内存分配失败
    }
    
    // 初始化基本字段
    node->c = c;
    node->num = 1;
    node->next = list;
    
    // 初始化HeffmanCode部分
    node->codeInfo.arraysize = 6;       // 修改此行
    node->codeInfo.codelen = 0;         // 修改此行
    node->codeInfo.code = (int*)malloc(6 * sizeof(int)); // 修改此行
    
    // 检查code分配是否成功
    if(node->codeInfo.code == NULL) {
        free(node);
        return list;
    }
    
    return node;
}
void extern_codearray(struct HeffmanCode* heffmancode){
    int* newcodearray = (int*)realloc(heffmancode->code,heffmancode->arraysize*2*sizeof(int));
    if(newcodearray==NULL){
        printf("[ERROR] failed to realloc the code array,int the struct HeffmanCode");
        //free all the pointer
        exit(1);
    }
    heffmancode->arraysize = heffmancode->arraysize * 2;
    heffmancode->code = newcodearray;
    return;
}

Charnode* findchar(Charnode* tab,unsigned char c){
    int cvalue = hash(c);
    Charnode *result = tab[cvalue].next;
    while(result!=NULL){
        if(result->c==c){
            return result;
        }
        result = result->next;
    }
    return NULL;
}

void free_Hashtab(Charnode* tab){
    for(int i=0;i<CAPACITY;++i){
        Charnode *nodetofree = &tab[i];
        while (tab[i].next!=NULL)
        {
            nodetofree = tab[i].next;
            tab[i].next = tab[i].next->next;
            free(nodetofree);
        }
    }
    free(tab);
}