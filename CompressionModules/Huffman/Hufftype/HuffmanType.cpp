#include "HuffmanType.h"

//method of CodeTable

CodeTable::CodeTable(){
   clear();
}

void CodeTable::clear(){
   // 只清 len[]/code[]：所有读取都以 len>0 为前提，packed[] 的陈旧内容不会被读到，
   // 保留其容量可避免每块 256 次重新分配。
   for(int i = 0; i < 256; ++i){
      len[i] = 0;
      code[i] = 0;
   }
}

//method of HuffTreeNode

HuffTreeNode::HuffTreeNode(
   const unsigned char data,
   FreqT freq,
   struct HuffTreeNode* left,
   struct HuffTreeNode* right,
   bool isLeaf
):
   data(data), freq(freq), left(left), right(right), isLeaf(isLeaf) { }

/*
inline HuffTreeNode::HuffTreeNode(
   const char c,
   FreqT freq,
   struct HuffTreeNode* left,
   struct HuffTreeNode* right,
   bool isLeaf=false
):
   HuffTreeNode((unsigned char)c, freq, left, right, isLeaf) { }
*/

//method of PathStack

void PathStack::push(int bit){
   // 确保codeBlocks足够大
   size_t index = codeLen / 8;
   if(index >= codeBlocks.size()){
      codeBlocks.resize(index + 1, 0);
   }

   codeBlocks[index] = codeBlocks[index] | (bit << (7 - codeLen % 8));
   ++codeLen;
}

void PathStack::pop(){
   if(codeLen == 0) return;

   size_t index = (codeLen - 1) / 8;
   if(index < codeBlocks.size()){
      int offset = 9 - (codeLen % 8);
      if(offset <= 8){
         codeBlocks[index] = codeBlocks[index] >> offset << offset;
      }
   }
   --codeLen;
}

void PathStack::writeCode(CodeTable& tab, unsigned char sym){
   const CodeLenT L = codeLen;
   tab.len[sym] = L;

   // 字节打包形态（等价于旧实现的 clear + 逐个 push_back），
   // 供 len>64 或位缓冲溢位时的回退路径使用。
   tab.packed[sym].assign(codeBlocks.begin(), codeBlocks.end());

   // 右对齐 uint64 码字：codeBlocks 是 MSB-first 打包，末尾字节可能含填充位或
   // 历史 pop 残留的脏位，统一右移掉。nBytes 上限 8 => L<=64 时不会溢出。
   if(L <= 64){
      const size_t nBytes = (static_cast<size_t>(L) + 7) / 8;
      uint64_t v = 0;
      for(size_t i = 0; i < nBytes && i < codeBlocks.size(); ++i){
         v = (v << 8) | codeBlocks[i];
      }
      const unsigned pad = static_cast<unsigned>(nBytes * 8 - L);
      tab.code[sym] = v >> pad;
   }else{
      tab.code[sym] = 0; // 不用，走 packed
   }
}

//method of BitHandler

void BitHandler::handle(CodeT& codeBlocks, CodeLenT codeLen, sfc::block_t& outBlock){
   uint8_t remainingBits = codeLen;  // 剩余比特数

   for(size_t i = 0; i < codeBlocks.size() && remainingBits > 0; ++i) {
      uint8_t codeBlock = codeBlocks[i];
      // 计算当前字节中有效的位数：取剩余位数和8的较小值
      uint8_t bitsInBlock = (remainingBits >= 8) ? 8 : remainingBits;

      // 处理当前字节的比特
      for(uint8_t bitIdx = 0; bitIdx < bitsInBlock; ++bitIdx) {
         // 从codeBlock中提取比特（从高位到低位）
         uint8_t bit = (codeBlock >> (7 - bitIdx)) & 1;

         // 将比特放入当前输出字节
         byte = (byte << 1) | bit;
         bitLen++;
         remainingBits--;

         // 当输出字节满8位时，写出
         if(bitLen == 8) {
            outBlock.push_back(byte);
            byte = 0;
            bitLen = 0;
            ++byteCount;
         }
      }
   }
}

void BitHandler::handleFast(uint64_t code, CodeLenT codeLen, sfc::block_t& outBlock){
   // 调用方保证 bitLen + codeLen <= 64
   uint64_t acc;
   unsigned n;
   if(bitLen == 0){
      // 单独分支：bitLen==0 时 (uint64_t)byte << 64 属于 UB，必须绕开
      acc = code;
      n = codeLen;
   }else{
      // 此分支下 codeLen <= 63 必然成立（bitLen>=1 且 bitLen+codeLen<=64）
      acc = (static_cast<uint64_t>(byte) << codeLen) | code;
      n = static_cast<unsigned>(bitLen) + codeLen;
   }

   // 按字节批量冲刷。byte/bitLen 的不变式与逐位路径完全一致：
   // byte 的低 bitLen 位为待输出数据
   while(n >= 8){
      outBlock.push_back(static_cast<unsigned char>(acc >> (n - 8)));
      n -= 8;
      ++byteCount;
   }

   bitLen = static_cast<uint8_t>(n);
   byte = static_cast<unsigned char>(n ? (acc & ((1ULL << n) - 1)) : 0);
}

void BitHandler::handle(unsigned char byteIn, std::vector<uint8_t>& path, uint8_t validBits){
   // 解压时,只读取指定的有效位数（用于处理最后一个字节的填充位）
   // 注意：编码时使用 handleLast() 会将有效位左移到高位
   // 所以部分字节中的有效位在高位，需要从高位读取
   // validBits 范围: 1-8
   if(validBits > 8) validBits = 8;
   if(validBits == 0) return;

   // 如果 validBits < 8，说明这是最后一个部分字节
   // 有效位在高位（由 handleLast() 左移产生）
   if(validBits < 8) {
      // 有效位在位置 [7 downto 8-validBits]
      for(int i = 7; i >= 8 - validBits; --i) {
         uint8_t bit = (byteIn >> i) & 1;
         path.push_back(bit);
      }
   } else {
      // validBits == 8，所有位都有效
      for(int i = 7; i >= 0; --i) {
         uint8_t bit = (byteIn >> i) & 1;
         path.push_back(bit);
      }
   }
}

void BitHandler::handleLast(){
   // 处理最后不足8位的字节
   if(bitLen > 0) {
      // 将剩余的位左移补齐到8位
      byte = byte << (8 - bitLen);
      valuedBits = bitLen;
   }
}