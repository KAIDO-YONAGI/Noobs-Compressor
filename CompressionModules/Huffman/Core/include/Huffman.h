#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "../../Hufftype/HuffmanType.h"
#include <memory>
#include <stack>

/**
 * Huffman类 压缩/解压处理模块
 *
 *     blockFreq：暂存频率表（统计当前块，finishFreqStat 时并入总表）
 *     totalFreq：总频率表
 *     codeTab：平坦符号编码表（码长由树导出，码字按 canonical 规则重排）
 *     treeRoot：编码树的根节点（仅压缩侧用于导出码长）
 *
 * 压缩：statisticFreq → finishFreqStat → genHufftree → saveCodeInTab（含 canonical 重排）
 *       → codeTableToPlatUchar（写 meta）→ encode
 * 解压：spawnCodeTable（由码长表重建 canonical 码 + 解码查找表）→ decode
 *
 * 解码已改为「canonical 码 + 12bit 主表查表」：每个符号一次数组访存，
 * 取代原先的「逐字节展开成位列表 + 逐位走树」。码长 > 12bit 的符号
 * （256 符号 + 8MB 块的真实数据下罕见）走 canonical 逐位回退，正确性不受影响。
 *
 * 码长表以 258 字节（'L' + maxCodeLen + 256 项）写入 metadata，取代原先
 * 2 字节/节点的树序列化（满字母表约 1023 字节）。
 */
class Huffman {

public:
    Huffman();
    ~Huffman();

    // 压缩调用：statisticFreq、finishFreqStat、genHufftree、saveCodeInTab、codeTableToPlatUchar、encode
    // 解压调用：spawnCodeTable、decode
    void statisticFreq(const sfc::block_t&);
    void encode(const sfc::block_t&, sfc::block_t&, BitHandler bitOutput = BitHandler());
    void decode(const sfc::block_t&, sfc::block_t&, BitHandler bitInput = BitHandler(), size_t maxOutputSize = SIZE_MAX);
    void finishFreqStat();
    void genHufftree();
    void saveCodeInTab();
    // 序列化码长表（取代原 treeToPlatUchar 的整树序列化）
    void codeTableToPlatUchar(sfc::block_t& outBlock);
    // 由码长表重建 canonical 码与解码查找表（取代原 spawnTree）
    void spawnCodeTable(const sfc::block_t& inBlock);

private:
    static constexpr int kLutBits = 12;            // 主表位宽
    static constexpr int kLutSize = 1 << kLutBits; // 4096 项
    static constexpr int kMaxCodeLen = 256;        // 256 符号的理论上限为 255

    FreqT blockFreq[256];  // 暂存频率表（单个块），finishFreqStat 时并入 totalFreq
    FreqT totalFreq[256];  // 总频率表
    CodeTable codeTab;     // 平坦符号编码表
    HuffTreeNode* treeRoot;
    PathStack pathStack;

    // ---- canonical 码与解码查找表（每块重建）----
    int maxCodeLen;                             // 当前块最大码长
    uint8_t codeLenTab[256];                    // 码长表快照（序列化用；0 = 该符号未出现）
    uint8_t sortedSymbols[256];                 // 按 (码长, 符号值) 升序
    uint64_t canonFirstCode[kMaxCodeLen + 1];   // 每个码长的首个 canonical 码字
    uint16_t canonFirstIdx[kMaxCodeLen + 1];    // 每个码长在 sortedSymbols 中的首下标
    uint16_t canonCount[kMaxCodeLen + 1];       // 每个码长的符号数
    uint16_t lut[kLutSize];                     // 12bit 主表：0 = 长码回退；(码长<<8)|符号

    std::unique_ptr<Minheap> genMinheap();
    void runSaveCodeInTab(HuffTreeNode* root);
    void destroyTree(HuffTreeNode* node);

    // 由 codeTab.len 构造 canonical 码，回写 codeTab.code / packed
    void buildCanonical();
    // 由 canonical 码构造 12bit 解码主表
    void buildDecodeLut();
};

#endif //HUFFMAN_H
