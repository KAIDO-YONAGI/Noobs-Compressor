cmake_minimum_required(VERSION 3.16)
project(SimpleFilesCompressorTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 本 CMakeLists 位于 BuildTest/tests/ 下（仓库根往下两级）；编译器在配置时通过
#   -DCMAKE_C_COMPILER=... -DCMAKE_CXX_COMPILER=...
# 指定（GUI 用同一套 MinGW：D:/qt/Tools/mingw1120_64/bin/{gcc,g++}.exe）

# ========================
# 核心库源清单与包含目录：与 GUI 构建共用 ../sfc_core.cmake 的唯一定义
# ========================
# 此前这里与 BuildTest/CMakeLists.txt 各抄一份 21 文件清单与 9 条 -I，漏改一处
# 就会出现"GUI 编得过、测试链不上"的假绿，现已收敛。
#
# 另注（历史隐式依赖已消除）：My_Aes.h 现在只依赖 Commons/include/RuntimeLibrary.h，
# 不再把 FileLibrary.h 作为传递性提供者；归档格式短名由消费者自行包含 FileLibrary.h。
# 因此"某个仓库根的直接子目录无意中出现在 -I 里兜底"这一脆弱前提已不再需要。
include("${CMAKE_CURRENT_LIST_DIR}/../sfc_core.cmake")
set(REPO_ROOT "${SFC_REPO_ROOT}")

include_directories(${SFC_CORE_INCLUDE_DIRS})

# ========================
# LTO 归档器修正（同 GUI 构建）
# 开启 LTO 后 GCC 使用 -fno-fat-lto-objects，对象只含 GIMPLE；普通 ar/ranlib
# 无法处理会打 "plugin needed to handle lto object"。换 gcc-ar/gcc-ranlib 即消除。
# ========================
if(NOT MSVC AND CMAKE_CXX_COMPILER_AR)
    set(CMAKE_AR "${CMAKE_CXX_COMPILER_AR}" CACHE FILEPATH "Archiver for LTO objects" FORCE)
    set(CMAKE_RANLIB "${CMAKE_CXX_COMPILER_RANLIB}" CACHE FILEPATH "Ranlib for LTO objects" FORCE)
endif()

# ========================
# 全部测试目标启用 LTO（与 GUI 发布构建口径一致）
# ========================
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# ========================
# 编译警告：只开警告，不开 -Werror
# 单人开发下 -Werror 会在升级编译器时把构建直接搞挂，收益不抵成本，故只要求
# 警告可见。（-Wswitch 由 -Wall 隐含，GUI 构建里另有显式 -Wswitch 用于策略接线）
# ========================
add_compile_options(-Wall -Wextra)

# ========================
# ctest 注册：一条命令跑全部 —— ctest --output-on-failure
# 单元测试的临时文件（如 test_flagtype.tmp）经 WORKING_DIRECTORY 落在构建目录内，
# 不再污染仓库根
# ========================
enable_testing()

set(SFC_TEST_WORKDIR "${CMAKE_CURRENT_BINARY_DIR}")

# ---- AES 单元测试（纯手写断言，零外部依赖） ----
add_executable(test_aes
    test_aes.cpp
    "${SFC_REPO_ROOT}/EncryptionModules/Aes/src/mainCircle.cpp"
    "${SFC_REPO_ROOT}/EncryptionModules/Aes/src/AesFunctions.cpp"
)
target_link_libraries(test_aes PRIVATE advapi32 bcrypt)
add_test(NAME unit_aes COMMAND test_aes)
set_tests_properties(unit_aes PROPERTIES WORKING_DIRECTORY "${SFC_TEST_WORKDIR}")

# ---- BufferPool 单元测试（纯手写断言，零外部依赖） ----
add_executable(test_bufferpool
    test_bufferpool.cpp
    "${SFC_REPO_ROOT}/BufferPool/BufferPool.cpp"
)
add_test(NAME unit_bufferpool COMMAND test_bufferpool)
set_tests_properties(unit_bufferpool PROPERTIES WORKING_DIRECTORY "${SFC_TEST_WORKDIR}")

# ---- Huffman 单元测试（纯手写断言，零外部依赖） ----
add_executable(test_huffman
    test_huffman.cpp
    "${SFC_REPO_ROOT}/CompressionModules/Huffman/Core/src/Huffman.cpp"
    "${SFC_REPO_ROOT}/CompressionModules/Huffman/Hufftype/HuffmanType.cpp"
)
add_test(NAME unit_huffman COMMAND test_huffman)
set_tests_properties(unit_huffman PROPERTIES WORKING_DIRECTORY "${SFC_TEST_WORKDIR}")

# ---- FlagType 读写往返单元测试（纯手写断言，零外部依赖） ----
# 只用 ToolClasses.h 的头文件内模板，链接 ToolClasses.cpp/EncodingUtils.cpp 以保持与其他测试一致
add_executable(test_flagtype
    test_flagtype.cpp
    "${SFC_REPO_ROOT}/CompressorFileSystem/Commons/src/ToolClasses.cpp"
    "${SFC_REPO_ROOT}/CompressorFileSystem/Commons/src/EncodingUtils.cpp"
)
add_test(NAME unit_flagtype COMMAND test_flagtype)
set_tests_properties(unit_flagtype PROPERTIES WORKING_DIRECTORY "${SFC_TEST_WORKDIR}")

# ---- SafeQueue 单元测试（纯手写断言，零外部依赖，多线程） ----
add_executable(test_safequeue
    test_safequeue.cpp
)
add_test(NAME unit_safequeue COMMAND test_safequeue)
set_tests_properties(unit_safequeue PROPERTIES WORKING_DIRECTORY "${SFC_TEST_WORKDIR}")

# ---- WriteSorter 单元测试（纯手写断言，零外部依赖） ----
add_executable(test_writesorter
    test_writesorter.cpp
)
add_test(NAME unit_writesorter COMMAND test_writesorter)
set_tests_properties(unit_writesorter PROPERTIES WORKING_DIRECTORY "${SFC_TEST_WORKDIR}")

# ========================
# compress/decompress/verify 三个子命令，直接编译核心库源文件（与 GUI 共用同一份
# SFC_CORE_SOURCES，无 Qt）
# ========================
add_executable(tool_archivebaseline
    tool_archivebaseline.cpp
    ${SFC_CORE_SOURCES}
)
target_link_libraries(tool_archivebaseline PRIVATE advapi32 bcrypt)

# ========================
# 端到端往返验收（含加密路径）
# ========================
# 样本集在测试运行时生成，不入库（仓库 .gitignore 的 *.txt 会吃掉夹具文件，
# 且固定夹具容易随格式演进腐化）。FIXTURES_SETUP/REQUIRED 保证 sample 先跑，
# 且三个模式各自写到 root/roundtrip-<mode>/ 互不碰撞，可并行。
# roundtrip 内部即"压缩→解压→逐文件逐字节校验"，与手动跑 verify 等价。
set(SFC_CTEST_FIXTURE "${SFC_TEST_WORKDIR}/ctest_fixture")

add_test(NAME fixture_sample
         COMMAND tool_archivebaseline sample "${SFC_CTEST_FIXTURE}")
set_tests_properties(fixture_sample PROPERTIES
    FIXTURES_SETUP sfc_sample
    WORKING_DIRECTORY "${SFC_TEST_WORKDIR}")

foreach(_sfc_mode pack huffman huffman-aes)
    string(REPLACE "-" "_" _sfc_mode_id "${_sfc_mode}")
    add_test(NAME e2e_roundtrip_${_sfc_mode_id}
             COMMAND tool_archivebaseline roundtrip "${SFC_CTEST_FIXTURE}" ${_sfc_mode})
    set_tests_properties(e2e_roundtrip_${_sfc_mode_id} PROPERTIES
        FIXTURES_REQUIRED sfc_sample
        WORKING_DIRECTORY "${SFC_TEST_WORKDIR}")
endforeach()
unset(_sfc_mode)
unset(_sfc_mode_id)
