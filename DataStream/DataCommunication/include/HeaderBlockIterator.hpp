#pragma once
#include "FileLibrary.h"
#include "FileDetails.h"
#include "ToolClasses.hpp"

class HeaderBlockIterator {
public:
    enum class State { BEFORE_START, IN_PROGRESS, COMPLETED };

    HeaderBlockIterator(std::vector<unsigned char>& buf, 
                       FileQueue& q,
                       std::vector<std::string>& paths)
        : m_buffer(buf), m_queue(q), 
          m_filePathToScan(paths) { 
        m_rootPath = createRootPath(paths); 
    }
    
    bool move_next();
    std::pair<fs::path, char> current() const { return m_current; }
    FileCount_uint get_root_entries_count() const { return m_rootEntries; }

private:
    // 使用您指定的路径转换实现
    fs::path createRootPath(std::vector<std::string>& paths) {
        Transfer transfer;
        if (paths.empty())
            throw std::runtime_error("No scan paths provided");
        return fs::path(transfer.transPath(paths[0])).parent_path();
    }

    void check_bounds(size_t required) const {
        if (m_bufferPtr + required > m_buffer.size()) {
            throw std::out_of_range("Buffer overflow at offset " + 
                std::to_string(m_bufferPtr) + "/" + 
                std::to_string(m_buffer.size()));
        }
    }

    template <typename T> T parse_num() {
        check_bounds(sizeof(T));
        T val;
        memcpy(static_cast<void*>(&val), 
               m_buffer.data() + m_bufferPtr, 
               sizeof(T));
        m_bufferPtr += sizeof(T);
        return val;
    }

    void parse_name_str(std::string& name) {
        FileNameSize_uint size = parse_num<FileNameSize_uint>();
        check_bounds(size);
        name.assign(reinterpret_cast<char*>(m_buffer.data() + m_bufferPtr), size);
        m_bufferPtr += size;
    }

    // 成员变量
    std::vector<unsigned char>& m_buffer;
    FileQueue& m_queue;
    std::vector<std::string>& m_filePathToScan;
    fs::path m_rootPath;
    size_t m_bufferPtr = 0;
    State m_state = State::BEFORE_START;
    std::pair<fs::path, char> m_current;
    FileCount_uint m_rootEntries = 0;
};
