#include "HeaderBlockIterator.hpp"
#include <iostream> // 用于调试输出

bool HeaderBlockIterator::move_next() {
    if (m_state == State::COMPLETED) return false;
    
    try {
        // ===== 第一阶段：根目录标识解析 =====
        if (m_state == State::BEFORE_START) {
            // 验证缓冲区有效性
            if (m_buffer.empty()) {
                throw std::runtime_error("Directory buffer is empty (0 bytes)");
            }
            
            // 检查根目录标识（必须为'3'）
            unsigned char rootFlag = parse_num<unsigned char>();
            if (rootFlag != '3') {
                throw std::runtime_error(
                    "Root flag missing. Expected '3', got '" + 
                    std::to_string(rootFlag) + "' at offset 0"
                );
            }
            
            // 解析根目录名称（如"YONAGI"）
            std::string rootName;
            parse_name_str(rootName);
            
            // 获取根目录下项目数量
            m_rootEntries = parse_num<FileCount_uint>();
            
            // 创建虚拟根目录项
            Directory_FileDetails root("ROOT:" + rootName, 0, 0, false, m_rootPath);
            m_queue.push({root, m_rootEntries});
            
            m_state = State::IN_PROGRESS;
            m_current = {m_rootPath, '3'};
            return true;
        }
        
        // ===== 第二阶段：条目解析 =====
        if (m_bufferPtr >= m_buffer.size()) {
            m_state = State::COMPLETED;
            return false;
        }
        
        // 读取条目类型标志
        const unsigned char flag = parse_num<unsigned char>();
        
        // 目录条目处理 (flag '0')
        if (flag == '0') {
            std::string dirName;
            parse_name_str(dirName);
            
            // 获取子目录/文件数量
            FileCount_uint childCount = parse_num<FileCount_uint>();
            
            // 构建完整路径
            fs::path fullPath = m_queue.empty() 
                ? m_rootPath / dirName 
                : m_queue.front().first.getFullPath() / dirName;
            
            // 添加到队列
            Directory_FileDetails dirDetails(dirName, 0, 0, false, fullPath);
            m_queue.push({dirDetails, childCount});
            
            m_current = {fullPath, '0'};
            return true;
        }
        // 文件条目处理 (flag '1')
        else if (flag == '1') {
            std::string fileName;
            parse_name_str(fileName);
            
            // 跳过文件大小信息
            m_bufferPtr += sizeof(FileSize_uint) * 2; // original + compressed
            
            // 构建完整路径
            fs::path fullPath = m_queue.empty() 
                ? m_rootPath / fileName 
                : m_queue.front().first.getFullPath() / fileName;
            
            m_current = {fullPath, '1'};
            return true;
        }
        // 非法标志处理
        else {
            throw std::runtime_error(
                "Invalid entry flag '" + std::to_string(flag) + 
                "' at offset " + std::to_string(m_bufferPtr - 1)
            );
        }
    }
    catch (const std::exception& e) {
        m_state = State::COMPLETED;
        throw std::runtime_error(
            "Parser failure at offset " + std::to_string(m_bufferPtr) + ": " + 
            e.what() + "\nBuffer size: " + std::to_string(m_buffer.size())
        );
    }
}
