#ifndef HEADERREADER
#define HEADERREADER

#include<fstream>
#include<filesystem>
#include<vector>
#include<iostream>
#include<cstring>
#include<string>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdint>

using namespace std;

namespace fs = std::filesystem; 

void readerForCompression();
void readerForDecompression();
void listFiles(const fs::path &basePath, const fs::path &relativePath, std::vector<std::string> &files);
void appendMagicStatic(const string& outputFilePath);
void outPutAllPaths(string &outPutFilePath, string &filePathToScan);
bool fileIsExist(string &outPutFilePath);
void scanFlow(string &outPutFilePath, string &filePathToScan);



#endif