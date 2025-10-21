#include "heffman.h"


Path* newPath(){
    Path *thepath = (Path*)malloc(sizeof(Path));
    int *path = (int*)malloc((STACK_CAPACITY)*sizeof(int));
    thepath->path = path;
    thepath->pathsize = 0;
    thepath->capacity = STACK_CAPACITY;
    return thepath;
}

void path_push(Path* path,int choose){
    if(choose!=0 && choose!=1){
        printf("[ERROR] in the method path_push,only receive 0 or 1 as the path\n");
    }
    if(path->pathsize>=path->capacity){
        path = extern_patharray(path);
    }
    path->path[path->pathsize] = choose;
    ++path->pathsize;
}

int path_pop(Path* path){
    int result = path->path[path->pathsize-1];
    path->path[path->pathsize-1] = -1;
    --path->pathsize;
    return result;
}

Path* extern_patharray(Path* path){
    int* newpatharray = (int*)realloc(path->path,path->capacity*2*sizeof(int));
    if(newpatharray==NULL){
        printf("[ERROR] failed to realloc the patharray,in the method extern_patharray\n");
        exit(1);
    }
    path->path = newpatharray;
    path->capacity = path->capacity * 2;
    return path;
}

void getpath(struct HeffmanCode* heffmancode,Path* path){
    for(int i=0;i<path->pathsize;++i){
        if(heffmancode->codelen>=heffmancode->arraysize){
            extern_codearray(heffmancode);
        }
        heffmancode->code[heffmancode->codelen++] = path->path[i];        
    }
}

// void save_heffmancode(Charnode* tab,Treenode* root,Path* path){
//     if(root->left==NULL&&root->right==NULL){
//         Charnode *charintab = findchar(tab,root->c);
//         getpath(&charintab->HeffmanCode,path);
//         return;
//     }
//     path_push(path,0);
//     save_heffmancode(tab,root->left,path);
//     path_pop(path);
//     path_push(path,1);
//     save_heffmancode(tab,root->right,path);
//     path_pop(path);
// }
void save_heffmancode(Charnode* tab, Treenode* root, Path* path) {
    if(root->left == NULL && root->right == NULL) {
        Charnode *charintab = findchar(tab, root->c);
        getpath(&charintab->codeInfo, path);  // ÐÞ¸Äµã1£ºHeffmanCode ¡ú codeInfo
        return;
    }
    
    path_push(path, 0);
    save_heffmancode(tab, root->left, path);
    path_pop(path);
    
    path_push(path, 1);
    save_heffmancode(tab, root->right, path);
    path_pop(path);
}



//chat to code method
void run_char_to_code(Charnode* tab,BitIOer* bw,FILE* orifile){
    rewind(orifile);
    int c_int = 0;
    while( (c_int = fgetc(orifile)) != EOF){
        char c = (char)c_int;
        Charnode* charnode = findchar(tab,c);
        if(charnode==NULL){
            printf("[ERROR] somthing wrong that can find %c in the hashtab,in method char_to_code\n",c);
            //free all the pointer ?
            exit(1);
        }
        for(int i=0;i<charnode->codeInfo.codelen;++i){
            writebit(bw,charnode->codeInfo.code[i]);
        }
    }
    bw->remainbit = 8 - bw->bitcount;
    flash_bit(bw);
    bw->totalbyte = ftell(bw->fileptr);
    fclose(orifile);
}

Treenode* encode(Charnode* tab,FILE* orifile){
    rewind(orifile);
    int ch = 0;
    while((ch = fgetc(orifile)) != EOF){
        char c = (char)ch;
        insert_hashtab(tab,c);
    }
    Heap *theheap = buildHeap(tab,CAPACITY);
    Treenode *root = spawn_heffmancode(theheap);
    Path *path = newPath();
    save_heffmancode(tab,root,path);
    //free some pointer
    free_Heap(theheap);
    rewind(orifile);
    return root;
}

void char_to_code(){
    while(1){
        printf("please input the original filename(filename.format) : ");
        unsigned char *orifilename = getstring();
        if(strcmp((const char*)orifilename,"end") == 0) { break; }
        Fileopen *fop = createFopen(orifilename);
        if(fop == NULL){
            continue;
        }
        FILE *orifile = fopen((const char*)fop->oriFile,"rb");
        DatfileHead *datfilehead = createDatFHead(fop);
        BitIOer* bw = newBitIOer(fop,"wb");
        Charnode *tab = newHashTab();
        Treenode *root = encode(tab,orifile);
        run_char_to_code(tab,bw,orifile);
        save_treeinfile(root, bw, fop, datfilehead);
        free_Tree(root);
        free_Hashtab(tab);
        deleteBitIOer(bw);
        freeFileopen(fop);
        printf("[kotori] compress sucessfully\n");
    }
}
//code to char method

void code_to_char(){
    while(1){
        printf("please input the code filename(filename without format) : ");
        unsigned char *filename = getstring();
        if(strcmp((const char*)filename,"end") == 0) { break; }
        Fileopen *fop = createdatFopen(filename);
        FILE *fileptr = fopen((const char*)fop->compressFile,"rb");
        if(fileptr==NULL){
            printf("[ERROR] failed to open %s",fop->compressFile);
            fclose(fileptr);
            exit(1);
        }
        FILE *treedat = fopen((const char*)fop->treedatFile,"rb");
        if(treedat==NULL){
            printf("[ERROR] failed to open %s",fop->treedatFile);
            fclose(fileptr);
            fclose(treedat);
            exit(1);
        }
        DatfileHead* dfhead = createEmptyFhead();
        load_filehead(dfhead, treedat);
        completeFop(fop, dfhead);
        FILE *result = fopen((const char*)fop->oriFile,"wb");
        if(result==NULL){
            printf("[ERROR] failed to open %s",fop->oriFile);
            fclose(fileptr);
            fclose(treedat);
            fclose(result);
            exit(1);
        }
        BitIOer *bw = newBitIOer(fop,"rb");
        Treenode *root = load_treedat(treedat,bw);
        run_code_to_char(root,bw,result);
        free_Tree(root);
        deleteBitIOer(bw);
        fclose(fileptr);
        fclose(treedat);
        fclose(result);
        freeFileopen(fop);
        freeFileHead(dfhead);
        printf("[kotori] decompress sucessfully!\n");
    }
}

//get bit from the coded file and walked the tree.while geting the leaf node,geting the char
//this method end until the bit get EOF.
void run_code_to_char(Treenode* root,BitIOer* bw,FILE* decodefile){
    int bit;
    Treenode *node = root;
    while( (bit=getbit(bw))!=-1 ){
        if(bit==0) node = node->left;
        else if(bit==1) node = node->right;
        else{
            printf("[ERROR] bit error in run_code_to_char\n");
        }
        if(node==NULL){
            printf("[ERROR] Invalid bit sequence reached null node\n");
            node = root;
        }
        if(node->left==NULL&&node->right==NULL){
            fputc(node->c,decodefile);
            fflush(decodefile);
            node = root;
        }
    }
    if(node!=root){
        printf("[warning] bit stream is not at the end,the file ending may not correct\n");
    }
    return;
}

//method to save tree

void save_treeinfile(Treenode *root,BitIOer* bw, Fileopen* fop, DatfileHead* dfhead){
    FILE *treedat = fopen((const char*)fop->treedatFile,"wb");
    if(treedat==NULL){
        printf("[ERROR] failed to open or create the file %s\n",fop->treedatFile);
        //free?
        exit(1);
    }
    save_filehead(dfhead, treedat);
    fwrite(&bw->totalbyte,sizeof(long int),1,treedat);
    fwrite(&bw->remainbit,sizeof(int),1,treedat);
    run_save_treeinfile(root,treedat);
    fclose(treedat);
}

void run_save_treeinfile(Treenode *root,FILE *treedat){
    if(root==NULL){
        fputc('0',treedat);
        return;
    }
    if(root->left==NULL && root->right==NULL){
        fputc('1',treedat);
        fputc(root->c,treedat);
    }else{
        fputc('2',treedat);
        run_save_treeinfile(root->left,treedat);
        run_save_treeinfile(root->right,treedat);
    }
}

Treenode* load_treedat(FILE* treedat,BitIOer* bw){
    fread(&bw->totalbyte,sizeof(long int),1,treedat);
    fread(&bw->remainbit,sizeof(int),1,treedat);
    return run_load_treedat(treedat);
}

Treenode* run_load_treedat(FILE* treedat){
    int type = fgetc(treedat);
    if(type == EOF) return NULL; 
    if(type == '0') return NULL;    
    Treenode* node = newTreenode();
    if(type == '1') {
        node->c = fgetc(treedat);
    } else if(type == '2') {
        node->left = run_load_treedat(treedat);
        node->right = run_load_treedat(treedat);
    }
    return node;
}

//free method
void free_Path(Path* thepath){
    free(thepath->path);
    free(thepath);
}
