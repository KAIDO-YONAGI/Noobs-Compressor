#include "heffman.h"


BitIOer* newBitIOer(Fileopen* fop, const char* mode){  // 改为unsigned char
    BitIOer *bw = (BitIOer*)malloc(sizeof(BitIOer));
    
    bw->bitcount = 0;
    bw->buffer = 0;
    bw->totalbyte = 0;
    bw->bytecount = 0;
    bw->remainbit = 0;
     bw->fileptr = fopen((const char*)fop->compressFile, (const char*)mode);  // 只转换filename
     // 添加强制转换
    if(bw->fileptr==NULL){
        printf("[ERROR] failed to open file to write,in method newBitIOer\n");
        exit(1);
    }
    return bw;
}


void writebit(BitIOer* bw,int bit){
    if(bit!=0&&bit!=1){
        printf("[ERROR] error in method writerbit , bit can only be 0 or 1\n");
        //free all the pointer ?
        exit(1);
    }
    bw->buffer = (bw->buffer << 1) | bit;
    ++bw->bitcount;
    if(bw->bitcount==8){
        fputc(bw->buffer,bw->fileptr);
        bw->bitcount = 0;
        bw->buffer = 0;
        ++bw->totalbyte;
    }
}

void flash_bit(BitIOer* bw){
    bw->buffer <<= (8 - bw->bitcount);
    fputc(bw->buffer,bw->fileptr);
    bw->bitcount = 0;
    bw->buffer = 0;
}

void deleteBitIOer(BitIOer* bw){
    fclose(bw->fileptr);
    free(bw);
}

int getbit(BitIOer* bw){
    if(bw->bitcount<=0){
        if(bw->bytecount>=bw->totalbyte) return -1;
        int ch;
        if((ch = fgetc(bw->fileptr))==EOF){
            return -1;
        }
        bw->buffer = (unsigned char)ch;
        bw->bitcount = 8;
        ++bw->bytecount;
    }
    if(bw->bytecount==bw->totalbyte && bw->bitcount<=bw->remainbit){
        return -1;
    }
    int bit = (bw->buffer >> 7) & 1;
    bw->buffer <<= 1;
    --bw->bitcount;
    return bit;
}