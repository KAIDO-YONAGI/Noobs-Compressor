#include "Heffman_type.h"

//method of Chardata

inline Chardata::Chardata(){
   freq = 0;
   codelen = 0;
}

inline void Chardata::add(){
   ++freq;
}

inline void Chardata::add(const Chardata& othercd){
   freq += othercd.freq;
}

//method of Hefftreenode

inline Hefftreenode::Hefftreenode(
   const unsigned char data, 
   freq_t freq, 
   struct Hefftreenode* left, 
   struct Hefftreenode* right, 
   bool isleaf=false
):
   data(data), freq(freq), left(left), right(right), isleaf(isleaf) { }

/*
inline Hefftreenode::Hefftreenode(
   const char c, 
   freq_t freq, 
   struct Hefftreenode* left, 
   struct Hefftreenode* right, 
   bool isleaf=false
):
   Hefftreenode((unsigned char)c, freq, left, right, isleaf) { }
*/

//method of pathStack

void PathStack::push(int bit){
   auto codeblock = codeblocks.at(codelen / 8);
   codeblock = codeblock | (bit << (7 - codelen % 8));
   ++codelen;
}

void PathStack::pop(){
   auto codeblock = codeblocks.at(codelen / 8);
   int offset = 9 - codelen % 8;
   codeblock = codeblock >> offset << offset;
   --codelen;
}

void PathStack::writecode(Chardata& cdata){
   for(auto stackblock : codeblocks){
      cdata.code.push_back(stackblock);
   }
   cdata.codelen = codelen;
}

//method of BitHandler

void BitHandler::handle(code_t& codeblocks, codelen_t codelen, sfc::block_t& out_block){
   for(auto codeblock: codeblocks)
   {
      while (codelen > 0)
      {
         uint8_t offset = 8 - bitlen;
         if(codelen - offset >= 0){
            byte = byte | (codeblock >> bitlen);
            codeblock = codeblock << offset;
            bitlen += offset;
         } else {
            byte = byte | (codeblock >> bitlen);
            bitlen += offset - codelen;
         }
         codelen -= offset;
         if(bitlen == 8){
            out_block.push_back(byte);
            ++bytecount;
         }
      }      
   }
}

void BitHandler::handle(unsigned char byte, std::vector<uint8_t> path){
   uint8_t gopath = 0;
   uint8_t valbit = 7;
   if(bytecount == 1){
      valbit = valued_bits;
   }
   for(int i=valbit; i>=0; --i)
   {
      gopath = gopath | (byte >> i);
      path.push_back(gopath);
   }
   --bytecount;
}