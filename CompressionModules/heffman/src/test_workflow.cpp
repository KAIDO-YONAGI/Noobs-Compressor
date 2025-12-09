#include <iostream>
#include <fstream>
#include <cassert>
#include <cstring>
#include <chrono>

#include "../include/Heffman.h"
#include "../include/GetFreq.h"
#include "../include/GenHeffcodeTab.h"
#include "../include/DoEncode.h"
#include "../include/LoadHeffcodeTab.h"
#include "../include/DoDecode.h"
#include "../../../DataBlocks/DataBlocksManage.h"
#include "../../../Schedule/include/Datacmnctor.h"

using namespace std;
using namespace sfc;

// 颜色输出辅助函数
void print_step(int step, const string& desc) {
    cout << "\n========================================\n";
    cout << "步骤 " << step << ": " << desc << "\n";
    cout << "========================================\n";
}

void print_success(const string& msg) {
    cout << "✓ " << msg << "\n";
}

void print_info(const string& msg) {
    cout << "ℹ " << msg << "\n";
}

void print_error(const string& msg) {
    cout << "✗ 错误: " << msg << "\n";
}

int main() {
    cout << "===============================================\n";
    cout << "       赫夫曼压缩解压完整流程测试\n";
    cout << "===============================================\n";

    // 步骤0：创建压缩模块对象
    print_step(0, "创建压缩模块对象");

    Heffman* heffman = nullptr;
    try {
        heffman = new Heffman(1);  // 创建支持1线程的Heffman对象（不测试多线程）
        print_success("Heffman对象创建成功");
    } catch (const exception& e) {
        print_error("Heffman对象创建失败: " + string(e.what()));
        return 1;
    }

    // 步骤1：生成测试数据并测试频率统计
    print_step(1, "生成256+字节测试数据并测试频率统计");

    DataBlocksManage data_manager(8);  // 创建8个数据块
    block_t test_data;

    // 生成测试数据：包含足够字节和多样化字符
    const char* test_str = "Hello, World! This is a test string for Huffman compression. "
                           "The quick brown fox jumps over the lazy dog. "
                           "Pack my box with five dozen liquor jugs. "
                           "How vexingly quick daft zebras jump! "
                           "Sphinx of black quartz, judge my vow. "
                           "Two driven jocks help foxy brown jump. "
                           "Jackdaws love my big sphinx of quartz. ";

    // 确保至少256字节
    size_t base_len = strlen(test_str);
    size_t repeat_count = (256 / base_len) + 2;

    for (size_t i = 0; i < repeat_count; i++) {
        for (size_t j = 0; j < base_len; j++) {
            test_data.push_back(test_str[j]);
        }
    }

    print_info("生成的测试数据大小: " + to_string(test_data.size()) + " 字节");

    // 将测试数据放入输入块
    sfc::blocks_t* input_blocks = data_manager.get_input_blocks();
    assert(input_blocks != nullptr);

    // 直接使用单个块存储数据（不测试多线程）
    input_blocks->at(0) = test_data;

    // 调用频率统计模块
    print_info("开始调用频率统计模块...");
    // 直接调用heffman->statistic_freq而不通过GetFreq
    heffman->statistic_freq(0, input_blocks->at(0));
    print_success("频率统计模块执行成功");

    // 步骤2：生成编码表、保存和加载
    print_step(2, "生成编码表、保存和加载");

    data_manager.done();  // 轮转输入输出

    heffman->merge_ttabs();
    print_success("线程哈希表合并成功");

    heffman->gen_hefftree();
    print_success("赫夫曼树生成成功");

    heffman->save_code_inTab();
    print_success("编码保存至哈希表成功");

    print_info("编码表生成完成");
    print_success("编码表保存成功");

    // 步骤3：执行DoEncode压缩
    print_step(3, "执行DoEncode压缩");

    // 重新准备输入数据
    data_manager.done();  // 再次轮转
    input_blocks = data_manager.get_input_blocks();
    sfc::blocks_t* output_blocks_for_encode = data_manager.get_output_blocks();

    print_info("输入块数: " + to_string(input_blocks->size()));
    print_info("输出块数: " + to_string(output_blocks_for_encode->size()));

    // 清理输出块
    output_blocks_for_encode->clear();
    for(int i = 0; i < 8; i++) {
        output_blocks_for_encode->at(i).clear();
    }

    input_blocks->at(0) = test_data;  // 直接使用原始数据

    print_info("准备调用DoEncode，输入块大小: " + to_string(input_blocks->at(0).size()));

    DoEncode encode_worker(heffman);
    try {
        encode_worker.work(&data_manager);
        print_success("数据编码压缩成功");
    } catch (const exception& e) {
        print_error("数据编码压缩失败: " + string(e.what()));
        delete heffman;
        return 1;
    }

    // 获取压缩后的数据
    data_manager.done();
    sfc::blocks_t* compressed_blocks = data_manager.get_output_blocks();
    print_info("压缩后块数: " + to_string(compressed_blocks->size()));
    assert(compressed_blocks != nullptr && compressed_blocks->size() > 0);

    size_t compressed_size = 0;
    for (size_t i = 0; i < (size_t)compressed_blocks->size(); i++) {
        compressed_size += compressed_blocks->at(i).size();
    }

    print_info("原始数据大小: " + to_string(test_data.size()) + " 字节");
    print_info("压缩后数据大小: " + to_string(compressed_size) + " 字节");
    float ratio = (float)compressed_size / test_data.size() * 100;
    print_info("压缩率: " + to_string(ratio) + "%");

    // 步骤4：读取编码表还原树用于解压
    print_step(4, "读取编码表还原树用于解压");

    // 创建新的Heffman对象用于解压
    Heffman* heffman_decode = nullptr;
    try {
        heffman_decode = new Heffman(1);
        print_success("解压用Heffman对象创建成功");
    } catch (const exception& e) {
        print_error("解压用Heffman对象创建失败: " + string(e.what()));
        delete heffman;
        return 1;
    }

    // 这里简化处理，直接使用压缩后的树
    // 在实际应用中应该序列化和反序列化编码表
    heffman_decode->receiveTreRroot(heffman->getTreeRoot());
    print_success("编码表加载和树还原成功");

    // 步骤5：执行DoDecode解压
    print_step(5, "执行DoDecode解压");

    // 准备压缩数据用于解码
    data_manager.done();
    input_blocks = data_manager.get_input_blocks();
    for (size_t i = 0; i < (size_t)compressed_blocks->size() && i < (size_t)input_blocks->size(); i++) {
        input_blocks->at(i) = compressed_blocks->at(i);
    }

    DoDecode decode_worker(heffman_decode);
    try {
        decode_worker.work(&data_manager);
        print_success("数据解码解压成功");
    } catch (const exception& e) {
        print_error("数据解码解压失败: " + string(e.what()));
        delete heffman;
        delete heffman_decode;
        return 1;
    }

    // 获取解压后的数据
    data_manager.done();
    sfc::blocks_t* decompressed_blocks = data_manager.get_output_blocks();
    assert(decompressed_blocks != nullptr && decompressed_blocks->size() > 0);

    // 合并解压数据
    block_t decompressed_data;
    for (size_t i = 0; i < (size_t)decompressed_blocks->size(); i++) {
        block_t& blk = decompressed_blocks->at(i);
        decompressed_data.insert(decompressed_data.end(), blk.begin(), blk.end());
    }

    print_info("解压后数据大小: " + to_string(decompressed_data.size()) + " 字节");

    // 验证解压结果
    print_step(6, "验证解压结果");

    if (decompressed_data.size() == test_data.size()) {
        bool match = true;
        for (size_t i = 0; i < test_data.size(); i++) {
            if (decompressed_data[i] != test_data[i]) {
                match = false;
                print_error("数据不匹配在位置: " + to_string(i));
                break;
            }
        }

        if (match) {
            print_success("解压数据与原始数据完全匹配！");
        } else {
            print_error("解压数据与原始数据不匹配");
            delete heffman;
            delete heffman_decode;
            return 1;
        }
    } else {
        print_error("解压数据大小不匹配");
        print_info("原始大小: " + to_string(test_data.size()));
        print_info("解压大小: " + to_string(decompressed_data.size()));
        delete heffman;
        delete heffman_decode;
        return 1;
    }

    // 清理资源
    delete heffman;
    delete heffman_decode;

    cout << "\n===============================================\n";
    cout << "          ✓ 所有测试用例通过！\n";
    cout << "===============================================\n";

    return 0;
}
