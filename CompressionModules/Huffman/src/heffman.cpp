#include "heffman.h"
//priority queue method
//o(nlgn)
Heap* buildHeap(Charnode* tab,int tabsize){
    Treenode **heap = NULL;
    int heapsize=0;
    for(int i=0;i<tabsize;++i){
        Charnode *cur = tab[i].next;
        while(cur!=NULL){
            ++heapsize;
            cur = cur->next;
        }
    }
    heap = (Treenode**)malloc(heapsize*sizeof(Treenode*));
    int j = 0;
    for(int i=0;i<tabsize;++i){
        Charnode *cur = tab[i].next;
        while(cur!=NULL){
            heap[j] = (Treenode*)malloc(sizeof(Treenode));
            heap[j]->key = cur->num;
            heap[j]->c = cur->c;
            heap[j]->left = NULL;
            heap[j]->right = NULL;
            ++j;
            cur = cur->next;
        }
    }
    Heap *theheap = (Heap*)malloc(sizeof(Heap));
    theheap->heap = heap;    
    theheap->size = heapsize;    
    for(int i=get_pr_index(heapsize-1);i>=0;--i){
        heapify(theheap,i);
    }
    return theheap;
}

//o(lgn)
void heapify(Heap* theheap,int parent){
    Treenode **heap = theheap->heap;
    int lc = get_lc_index(parent);
    int rc = get_rc_index(parent);
    int min = parent;
    if(lc<theheap->size && heap[lc]->key<heap[parent]->key)
        min = lc;
    if(rc<theheap->size && heap[rc]->key<heap[min]->key)
        min = rc;
    if(min!=parent){
        exchange(heap,parent,min);
        heapify(theheap,min);
    }
}

//o(lgn)
void swim(Heap* theheap, int child){
    Treenode **heap = theheap->heap;
    int parent = get_pr_index(child);
    while (child > 0 && heap[child]->key < heap[parent]->key) {
        exchange(heap, child, parent);
        child = parent;
        parent = get_pr_index(child);
    }
}

Treenode* dequeue(Heap* theheap){
    if(theheap->size<=0) return NULL;
    Treenode **heap = theheap->heap;
    Treenode *pop = heap[0];
    exchange(heap,0,theheap->size-1);
    --theheap->size;
    heapify(theheap,0);
    return pop;
}

void enqueue(Heap* theheap,Treenode* data){
    Treenode **heap = theheap->heap;
    heap[theheap->size] = data;
    ++theheap->size;
    swim(theheap,theheap->size-1);
}

int get_lc_index(int parent){
    return (parent+1)*2-1;
}

int get_rc_index(int parent){
    return (parent+1)*2;
}

int get_pr_index(int child){
    return (child-1)/2;
}

void exchange(Treenode** heap,int index1,int index2){
    Treenode *mid = heap[index1];
    heap[index1] = heap[index2];
    heap[index2] = mid;
}

//heffman tree method
//o(nlgn)
Treenode* spawn_heffmancode(Heap* theheap){
    int n = theheap->size;
    for(int i=1;i<=n-1;++i){
        Treenode *root = newTreenode();
        root->left = dequeue(theheap);
        root->right = dequeue(theheap);
        root->key = root->left->key + root->right->key;
        enqueue(theheap,root);
    }
    return dequeue(theheap);
}

Treenode* newTreenode(){
    Treenode *node = (Treenode*)malloc(sizeof(Treenode));
    node->left = NULL;
    node->right = NULL;
    node->key = 0;
    return node;
}

//free method
void free_Tree(Treenode* root){
    if(root==NULL){
        return;
    }
    free_Tree(root->left);
    free_Tree(root->right);
    free(root);
}

void free_Heap(Heap* heap){
    free(heap->heap);
    free(heap);
}