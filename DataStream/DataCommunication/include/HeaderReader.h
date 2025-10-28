#ifndef HEADERREADER
#define HEADERREADER

#include<fstream>
#include<filesystem>
#include<vector>
#include<iostream>
#include<cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdint>
using namespace std;

void readerForCompression();
void readerForDecompression();
void listFiles(const string &basePath, const string &relativePath, vector<string> &files);
void appendMagicStatic(const string& outputFilePath);
void outPutAllPaths(string &outPutFilePath, string &filePathToScan);
bool fileIsExist(string &outPutFilePath);
void scanFlow(string &outPutFilePath, string &filePathToScan);



#endif