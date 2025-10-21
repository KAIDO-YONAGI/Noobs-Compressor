#include "heffman.h"

//create when compress
Fileopen* createFopen(unsigned char* wholefilename) {
    Fileopen *Fopen = (Fileopen*)malloc(sizeof(Fileopen));
    if (Fopen == NULL) {
        perror("Failed to allocate Fileopen");
        return NULL;
    }

    // --- Process original filename to get base name and extension ---
    // Make a copy of wholefilename because strtok modifies the string
    unsigned char *wholefilename_copy = (unsigned char *)strdup((const char *)wholefilename);

    if (wholefilename_copy == NULL) {
        perror("Failed to duplicate wholefilename");
        free(Fopen);
        return NULL;
    }

    unsigned char *token_filename = (unsigned char *)strtok((char*)wholefilename_copy, ".");
    if (token_filename != NULL) {
        Fopen->filename = (unsigned char *)strdup((const char*)token_filename); // Duplicate filename
        if (Fopen->filename == NULL) {
            perror("Failed to duplicate filename");
            free(wholefilename_copy);
            free(Fopen);
            return NULL;
        }
    } else {
        // Handle case where there's no extension (e.g., "myfile")
        Fopen->filename = (unsigned char *)strdup((const char*)wholefilename); // Treat wholefilename as filename
        if (Fopen->filename == NULL) {
            perror("Failed to duplicate filename (no extension)");
            free(wholefilename_copy);
            free(Fopen);
            return NULL;
        }
        Fopen->openformat = NULL; // No openformat
    }

    unsigned char *token_format = (unsigned char *)strtok((char*)NULL, ".");
    if (token_format != NULL) {
        Fopen->openformat = (unsigned char *)strdup((const char*)token_format); // Duplicate openformat
        if (Fopen->openformat == NULL) {
            perror("Failed to duplicate openformat");
            free(Fopen->filename);
            free(wholefilename_copy);
            free(Fopen);
            return NULL;
        }
    } else if (token_format == NULL) {
        printf("[Error] cannot get the format of file. check if you input 'filename.format'\n");
        return NULL;
        // If filename had no extension, openformat is already NULL
        // If filename had an extension but it was the last token, this is fine
    }

    free(wholefilename_copy); // Free the duplicated string used by strtok

    // --- Construct full paths ---
    // Consider adding constants for directory prefixes to improve readability
    const unsigned char *ORIGINAL_DIR = (unsigned char *)"originalfile/";
    const unsigned char *COMPRESS_DIR = (unsigned char *)"after_compress/";
    const unsigned char *DAT_EXT = (unsigned char *)".dat";
    const unsigned char *TD_EXT = (unsigned char *)"td.dat";

    // Allocate and snprintf oriFile
    int len_orifile_path = strlen((const char*)ORIGINAL_DIR) + strlen((const char*)wholefilename) + 1; // +1 for null terminator
    Fopen->oriFile = (unsigned char*)malloc(len_orifile_path * sizeof(unsigned char));
    if (Fopen->oriFile == NULL) {
        perror("Failed to allocate oriFile path");
        // Free previously allocated memory before returning
        free(Fopen->openformat);
        free(Fopen->filename);
        free(Fopen);
        return NULL;
    }
    snprintf((char*)Fopen->oriFile, len_orifile_path, "%s%s", ORIGINAL_DIR, wholefilename);

    // Allocate and snprintf compressFile
    int len_cpfile_path = strlen((const char*)COMPRESS_DIR) + strlen((const char*)Fopen->filename) + strlen((const char*)DAT_EXT) + 1;
    Fopen->compressFile = (unsigned char*)malloc(len_cpfile_path * sizeof(unsigned char));
    if (Fopen->compressFile == NULL) {
        perror("Failed to allocate compressFile path");
        // Free previously allocated memory
        free(Fopen->oriFile);
        free(Fopen->openformat);
        free(Fopen->filename);
        free(Fopen);
        return NULL;
    }
    snprintf((char*)Fopen->compressFile, len_cpfile_path, "%s%s%s", COMPRESS_DIR, Fopen->filename, DAT_EXT);

    // Allocate and snprintf treedatFile
    int len_tdfile_path = strlen((const char*)COMPRESS_DIR) + strlen((const char*)Fopen->filename) + strlen((const char*)TD_EXT) + 1;
    Fopen->treedatFile = (unsigned char*)malloc(len_tdfile_path * sizeof(unsigned char));
    if (Fopen->treedatFile == NULL) {
        perror("Failed to allocate treedatFile path");
        // Free previously allocated memory
        free(Fopen->compressFile);
        free(Fopen->oriFile);
        free(Fopen->openformat);
        free(Fopen->filename);
        free(Fopen);
        return NULL;
    }
    snprintf((char*)Fopen->treedatFile, len_tdfile_path, "%s%s%s", COMPRESS_DIR, Fopen->filename, TD_EXT);

    return Fopen;
}

//create when decompress
Fileopen* createdatFopen(unsigned char* base_filename) {
    Fileopen *Fopen = (Fileopen*)malloc(sizeof(Fileopen));
    if (Fopen == NULL) {
        perror("Failed to allocate Fileopen");
        return NULL;
    }

    // Duplicate the base filename
    Fopen->filename = (unsigned char *)strdup((const char*)base_filename);
    if (Fopen->filename == NULL) {
        perror("Failed to duplicate base_filename");
        free(Fopen);
        return NULL;
    }

    // In decompression, we don't have the original 'openformat' directly passed.
    // It's assumed to be part of the metadata stored in the compressed file
    // or implicitly handled during original file reconstruction.
    // For now, we set it to NULL, or you could add a placeholder/logic to read it from compressed file later.
    Fopen->openformat = NULL; // We don't have the openformat at this stage from arguments

   
    // const unsigned char *ORIGINAL_DIR = (unsigned char *)"originalfile/";
    const unsigned char *COMPRESS_DIR = (unsigned char *)"after_compress/";
    const unsigned char *DAT_EXT = (unsigned char *)".dat";
    const unsigned char *TD_EXT = (unsigned char *)"td.dat";

    // Allocate and snprintf compressFile (input for decompression)
    // This is the file that will be read for decompression.
    int len_cpfile_path = strlen((const char*)COMPRESS_DIR) + strlen((const char*)Fopen->filename) + strlen((const char*)DAT_EXT) + 1;
    Fopen->compressFile = (unsigned char*)malloc(len_cpfile_path * sizeof(unsigned char));
    if (Fopen->compressFile == NULL) {
        perror("Failed to allocate compressFile path");
        free(Fopen->filename);
        free(Fopen);
        return NULL;
    }
    snprintf((char*)Fopen->compressFile, len_cpfile_path, "%s%s%s", COMPRESS_DIR, Fopen->filename, DAT_EXT);

    // Allocate and snprintf treedatFile (input for decompression)
    // This is the file containing the Huffman tree structure.
    int len_tdfile_path = strlen((const char*)COMPRESS_DIR) + strlen((const char*)Fopen->filename) + strlen((const char*)TD_EXT) + 1;
    Fopen->treedatFile = (unsigned char*)malloc(len_tdfile_path * sizeof(unsigned char));
    if (Fopen->treedatFile == NULL) {
        perror("Failed to allocate treedatFile path");
        free(Fopen->compressFile);
        free(Fopen->filename);
        free(Fopen);
        return NULL;
    }
    snprintf((char*)Fopen->treedatFile, len_tdfile_path, "%s%s%s", COMPRESS_DIR, Fopen->filename, TD_EXT);

    // For oriFile, the full original filename (including extension)
    // would typically be stored within the compressed file's header.
    // At this point, we only have the base_filename.
    // We'll leave oriFile as NULL or a placeholder, and it should be
    // populated later during the decompression process after reading metadata.
    Fopen->oriFile = NULL; // Placeholder, to be set after reading compressed file header

    return Fopen;
}

void completeFop(Fileopen* fop, DatfileHead* dfhead){
    fop->openformat = (unsigned char*)malloc((dfhead->len + 1) * sizeof(unsigned char));
    strcpy((char*)fop->openformat, (const char*)dfhead->fileformat);
    fop->oriFile = (unsigned char*)malloc((13 + dfhead->len + strlen((const char*)fop->filename) + 2) * sizeof(unsigned char));
    snprintf((char*)fop->oriFile, 13 + dfhead->len + strlen((const char*)fop->filename) + 2, "originalfile/%s.%s",fop->filename, fop->openformat);
}

//create when compress
DatfileHead* createDatFHead(Fileopen* fop){
    DatfileHead* dfhead = (DatfileHead*)malloc(sizeof(*dfhead));
    dfhead->len = strlen((char *)fop->openformat);
    dfhead->fileformat = (unsigned char*)malloc((dfhead->len + 1) * sizeof(unsigned char));
    strcpy((char*)dfhead->fileformat, (const char*)fop->openformat);
    return dfhead;
}

//creatr when decompress
DatfileHead* createEmptyFhead(){
    DatfileHead* dfhead = (DatfileHead*)malloc(sizeof(*dfhead));
    return dfhead;
}

void save_filehead(DatfileHead* dfhead, FILE* tdfp){
    fputc('#', tdfp);
    fwrite(&dfhead->len, sizeof(int), 1, tdfp);
    fwrite(dfhead->fileformat, sizeof(unsigned char), dfhead->len, tdfp);
    fputc('#', tdfp);
    return;
}

void load_filehead(DatfileHead* dfhead, FILE* tdfp){
    if(fgetc(tdfp) != '#'){
        printf("[Error] error in reading filehead\n");
        exit(-1);
    }
    fread(&dfhead->len, sizeof(int), 1, tdfp);
    dfhead->fileformat = (unsigned char*)malloc(dfhead->len * sizeof(unsigned char));
    fread(dfhead->fileformat, sizeof(unsigned char), dfhead->len, tdfp);
    if(fgetc(tdfp) != '#'){
        printf("[Error] the format of filehead is wrong\n");
        free(dfhead);
        exit(-1);
    }
    return;
}

unsigned char* getstring(){
    int maxsize = 20;
    unsigned char* str = (unsigned char*)malloc(maxsize * sizeof(unsigned char));
    int len = 0;
    unsigned char c;
    while( (c = getchar()) != '\n' ){
        str[len++] = c;
        if(len >= maxsize - 1){
            maxsize *= 2;
            str = (unsigned char*)realloc(str, maxsize);
        }
    }
    str[len] = '\0';
    return str;
}

void freeFileopen(Fileopen* fop){
    free(fop->compressFile);
    free(fop->filename);
    free(fop->openformat);
    free(fop->oriFile);
    free(fop->treedatFile);
    free(fop);
}

void freeFileHead(DatfileHead* dfhead){
    free(dfhead->fileformat);
    free(dfhead);
}
