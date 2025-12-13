#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctime>
#define CAPACITY 61
#define STACK_CAPACITY 32

typedef struct{
    FILE *fileptr;
    unsigned char buffer;
    int bitcount;
    long int totalbyte;
    long int bytecount;
    int remainbit;
}BitIOer;
typedef struct HeffmanCode {
    int *code;
    int codelen;
    int arraysize;
}HeffmanCode;

typedef struct Charnode
{
    unsigned char c;
    int num;
    struct Charnode *next;
    HeffmanCode codeInfo; 
} Charnode;
typedef struct Fileopen
{
    unsigned char* openformat;
    unsigned char* filename;

    unsigned char* oriFile;
    unsigned char* compressFile;
    unsigned char* treedatFile;

}Fileopen;

typedef struct DatfileHead
{
    unsigned char* fileformat;
    int len;
}DatfileHead;

typedef struct Treenode
{
    unsigned char c;
    struct Treenode *left;
    struct Treenode *right;
    int key;
}Treenode;

typedef struct Heap
{
    Treenode **heap;
    int size;
}Heap;

typedef struct{
    int *path;
    int pathsize;
    int capacity;
}Path;



Path* newPath();
void path_push(Path* path,int choose);
int path_pop(Path* path);
Path* extern_patharray(Path* path);
void getpath(struct HeffmanCode* code,Path* path);
void save_heffmancode(Charnode* tab,Treenode* root,Path* path);



void run_char_to_code(Charnode* tab,BitIOer* bw,FILE* orifile);
Treenode* encode(Charnode* tab,FILE* orifile);
void char_to_code();


void code_to_char();
void run_code_to_char(Treenode* root,BitIOer* bw,FILE* decodefile);


void save_treeinfile(Treenode *root,BitIOer* bw, Fileopen* fop, DatfileHead* dfhead);
void run_save_treeinfile(Treenode *root,FILE *treedat);
Treenode* load_treedat(FILE* treedat,BitIOer* bw);
Treenode* run_load_treedat(FILE* treedat);


void free_Path(Path* thepath);


BitIOer* newBitIOer(Fileopen* fop, const char* mode);
void writebit(BitIOer* bw,int bit);
void flash_bit(BitIOer* bw);
void deleteBitIOer(BitIOer* bw);
int getbit(BitIOer* bw);


int hash(unsigned char c);
Charnode* newHashTab();
void insert_hashtab(Charnode* tab,unsigned char c);
Charnode* insert_list(Charnode* list,unsigned char c);
void extern_codearray(struct HeffmanCode* heffmancode);
Charnode* findchar(Charnode* tab,unsigned char c);

void free_Hashtab(Charnode* tab);



Fileopen* createFopen(unsigned char* wholefilename);
Fileopen* createdatFopen(unsigned char* filename);
void completeFop(Fileopen* fop, DatfileHead* dfhead);
DatfileHead* createDatFHead(Fileopen* fop);
DatfileHead* createEmptyFhead();
void save_filehead(DatfileHead* dfhead, FILE* tdfp);
void load_filehead(DatfileHead* dfhead, FILE* tdfp);
unsigned char* getstring();

void freeFileopen(Fileopen* fop);
void freeFileHead(DatfileHead* dfhead);



Heap* buildHeap(Charnode* tab,int tabsize);
void heapify(Heap* theheap,int parent);
void swim(Heap* theheap,int child);
Treenode* dequeue(Heap* theheap);
void enqueue(Heap* heap,Treenode* data);
int get_lc_index(int parent);
int get_rc_index(int parent);
int get_pr_index(int child);
void exchange(Treenode** heap,int index1,int index2);


Treenode* spawn_heffmancode(Heap* theheap);
Treenode* newTreenode();


void free_Tree(Treenode* root);
void free_Heap(Heap* heap);

int merge_files_with_length(const unsigned char* file1_path, const unsigned char* file2_path, const unsigned char* output_path);
int split_files_with_length(const unsigned char* merged_path, const unsigned char* file1_path, const unsigned char* file2_path);
