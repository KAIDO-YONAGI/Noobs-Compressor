cmake_minimum_required(VERSION 3.10)
project(HeffmanCompressionTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加项目根目录
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# 包含目录
include_directories(
    "${PROJECT_ROOT}/CompressionModules/heffman/include"
    "${PROJECT_ROOT}/CompressionModules/hefftype"
    "${PROJECT_ROOT}/DataBlocks"
    "${PROJECT_ROOT}/Schedule/include"
    "${PROJECT_ROOT}/ThreadPool"
    "${PROJECT_ROOT}/namespace"
)

# 源文件
set(HEFFMAN_SOURCES
    "${PROJECT_ROOT}/CompressionModules/heffman/src/Heffman.cpp"
    "${PROJECT_ROOT}/CompressionModules/heffman/src/GetFreq.cpp"
    "${PROJECT_ROOT}/CompressionModules/heffman/src/GenHeffcodeTab.cpp"
    "${PROJECT_ROOT}/CompressionModules/heffman/src/DoEncode.cpp"
    "${PROJECT_ROOT}/CompressionModules/heffman/src/LoadHeffcodeTab.cpp"
    "${PROJECT_ROOT}/CompressionModules/heffman/src/DoDecode.cpp"
    "${PROJECT_ROOT}/CompressionModules/heffman/src/GetFreq.cpp"
    "${PROJECT_ROOT}/CompressionModules/hefftype/Heffman_type.cpp"
    "${PROJECT_ROOT}/DataBlocks/DataBlocksManage.cpp"
    "${PROJECT_ROOT}/ThreadPool/src/ThreadPool.cpp"
    "${PROJECT_ROOT}/ThreadPool/src/_Thread.cpp"
    "${PROJECT_ROOT}/ThreadPool/src/MonitorTaskQueue.cpp"

)

# 频率统计辅助文件
set(FREQ_SOURCES
    "${PROJECT_ROOT}/CompressionModules/heffman/src/GetFreq.cpp"
)

# 编码表生成文件
set(CODEGEN_SOURCES
    "${PROJECT_ROOT}/CompressionModules/heffman/src/GenHeffcodeTab.cpp"
)

# 编码文件
set(ENCODE_SOURCES
    "${PROJECT_ROOT}/CompressionModules/heffman/src/DoEncode.cpp"
)

# 解码表加载文件
set(LOADCODE_SOURCES
    "${PROJECT_ROOT}/CompressionModules/heffman/src/LoadHeffcodeTab.cpp"
)

# 解码文件
set(DECODE_SOURCES
    "${PROJECT_ROOT}/CompressionModules/heffman/src/DoDecode.cpp"
)

# 完整工作流测试
add_executable(test_workflow test_workflow.cpp ${HEFFMAN_SOURCES})
target_link_libraries(test_workflow pthread)

# 其他必要的输出
message(STATUS "赫夫曼压缩测试项目已配置")
message(STATUS "项目根目录: ${PROJECT_ROOT}")
message(STATUS "编译目标: test_workflow")
