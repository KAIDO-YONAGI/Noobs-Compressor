#include "Heffman_type.h"

//method of Chardata

Chardata::Chardata(){
   freq = 0;
   codelen = 0;
}

void Chardata::add(){
   ++freq;
}

void Chardata::add(const Chardata& othercd){
   freq += othercd.freq;
}

//method of Hefftreenode

Hefftreenode::Hefftreenode(
   const unsigned char data,
   freq_t freq,
   struct Hefftreenode* left,
   struct Hefftreenode* right,
   bool isleaf
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
   // 确保codeblocks足够大
   size_t index = codelen / 8;
   if(index >= codeblocks.size()){
      codeblocks.resize(index + 1, 0);
   }

   codeblocks[index] = codeblocks[index] | (bit << (7 - codelen % 8));
   ++codelen;
}

void PathStack::pop(){
   if(codelen == 0) return;

   size_t index = (codelen - 1) / 8;
   if(index < codeblocks.size()){
      int offset = 9 - (codelen % 8);
      if(offset <= 8){
         codeblocks[index] = codeblocks[index] >> offset << offset;
      }
   }
   --codelen;
}

void PathStack::writecode(Chardata& cdata){
   // 清空之前的编码
   cdata.code.clear();

   // 写入新的编码
   for(auto stackblock : codeblocks){
      cdata.code.push_back(stackblock);
   }
   cdata.codelen = codelen;
}

//method of BitHandler

void BitHandler::handle(code_t& codeblocks, codelen_t codelen, sfc::block_t& out_block){
   uint8_t remaining_bits = codelen;  // 剩余比特数

   for(size_t i = 0; i < codeblocks.size() && remaining_bits > 0; ++i) {
      uint8_t codeblock = codeblocks[i];
      // 计算当前字节中有效的位数：取剩余位数和8的较小值
      uint8_t bits_in_block = (remaining_bits >= 8) ? 8 : remaining_bits;

      // 处理当前字节的比特
      for(uint8_t bit_idx = 0; bit_idx < bits_in_block; ++bit_idx) {
         // 从codeblock中提取比特（从高位到低位）
         uint8_t bit = (codeblock >> (7 - bit_idx)) & 1;

         // 将比特放入当前输出字节
         byte = (byte << 1) | bit;
         bitlen++;
         remaining_bits--;

         // 当输出字节满8位时，写出
         if(bitlen == 8) {
            out_block.push_back(byte);
            byte = 0;
            bitlen = 0;
            ++bytecount;
         }
      }
   }
}

void BitHandler::handle(unsigned char byte_in, std::vector<uint8_t>& path){
   // 解压时,总是读取所有8位
   // 通过decode函数的maxOutputSize参数来控制输出大小
   for(int i = 7; i >= 0; --i)
   {
      uint8_t bit = (byte_in >> i) & 1;
      path.push_back(bit);
   }
}

void BitHandler::handle_last(){
   // 处理最后不足8位的字节
   if(bitlen > 0) {
      // 将剩余的位左移补齐到8位
      byte = byte << (8 - bitlen);
      valued_bits = bitlen;
   }
}