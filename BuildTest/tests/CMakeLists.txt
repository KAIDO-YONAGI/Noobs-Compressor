cmake_minimum_required(VERSION 3.16)
project(SimpleFilesCompressorTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 本 CMakeLists 位于 BuildTest/tests/ 下（仓库根往下两级）；编译器在配置时通过
#   -DCMAKE_C_COMPILER=... -DCMAKE_CXX_COMPILER=...
# 指定（GUI 用同一套 MinGW：D:/qt/Tools/mingw1120_64/bin/{gcc,g++}.exe）
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# 说明：My_Aes.h 内部使用了 "../../../CompressorFileSystem/Commons/include/FileLibrary.h"
# 这种相对包含，需要某个"仓库根的直接子目录"出现在 -I 中才能解析
# （GUI 构建里历来由 DataBlocks//Schedule//Y_Manager 等目录无意中承担此角色，删除后由
#   Y_Manager/ 与 BufferPool/ 继续 兜底，此处同样给出）。
include_directories(
    "${REPO_ROOT}/EncryptionModules/Aes/include"
    "${REPO_ROOT}/CompressorFileSystem/Commons/include"
    "${REPO_ROOT}/CompressorFileSystem/DataIO/include"
    "${REPO_ROOT}/CompressorFileSystem/ArchiveFormat/include"
    "${REPO_ROOT}/CompressorFileSystem/Strategy/include"
    "${REPO_ROOT}/CompressionModules/Huffman/Core/include"
    "${REPO_ROOT}/CompressionModules/Huffman/Hufftype"
    "${REPO_ROOT}/BufferPool"
    "${REPO_ROOT}/Y_Manager"
)

# ========================
# AES 单元测试（纯手写断言，零外部依赖）
# ========================
add_executable(test_aes
    test_aes.cpp
    "${REPO_ROOT}/EncryptionModules/Aes/src/mainCircle.cpp"
    "${REPO_ROOT}/EncryptionModules/Aes/src/AesFunctions.cpp"
)
target_link_libraries(test_aes PRIVATE advapi32)

# ========================
# BufferPool 单元测试（纯手写断言，零外部依赖）
# ========================
add_executable(test_bufferpool
    test_bufferpool.cpp
    "${REPO_ROOT}/BufferPool/BufferPool.cpp"
)

# ========================
# Huffman 单元测试（纯手写断言，零外部依赖）
# ========================
add_executable(test_huffman
    test_huffman.cpp
    "${REPO_ROOT}/CompressionModules/Huffman/Core/src/Huffman.cpp"
    "${REPO_ROOT}/CompressionModules/Huffman/Hufftype/HuffmanType.cpp"
)

# ========================
# FlagType 读写往返单元测试（纯手写断言，零外部依赖）
# 只用 ToolClasses.h 的头文件内模板，链接 ToolClasses.cpp/EncodingUtils.cpp 以保持与其他测试一致
# ========================
add_executable(test_flagtype
    test_flagtype.cpp
    "${REPO_ROOT}/CompressorFileSystem/Commons/src/ToolClasses.cpp"
    "${REPO_ROOT}/CompressorFileSystem/Commons/src/EncodingUtils.cpp"
)

# ========================
# 归档基准 CLI 工具（重构验收用，见 DevFiles《文件系统类型与布局重构方案》§0）
# compress/decompress/verify 三个子命令，直接编译核心库源文件（与 GUI 的 CORE_SOURCES 一致，无 Qt）
# ========================
add_executable(tool_archivebaseline
    tool_archivebaseline.cpp
    "${REPO_ROOT}/CompressorFileSystem/Commons/src/EncodingUtils.cpp"
    "${REPO_ROOT}/CompressorFileSystem/Commons/src/FileSystemUtils.cpp"
    "${REPO_ROOT}/CompressorFileSystem/Commons/src/ToolClasses.cpp"
    "${REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/EntryParser.cpp"
    "${REPO_ROOT}/CompressorFileSystem/DataIO/src/DataExporter.cpp"
    "${REPO_ROOT}/CompressorFileSystem/DataIO/src/DataLoader.cpp"
    "${REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/EntryProcessor.cpp"
    "${REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/BinaryStandardWriter.cpp"
    "${REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/BinaryStandardLoader.cpp"
    "${REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/CatalogFinalizer.cpp"
    "${REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/HeaderWriter.cpp"
    "${REPO_ROOT}/CompressorFileSystem/Strategy/src/NullCompression.cpp"
    "${REPO_ROOT}/CompressorFileSystem/Strategy/src/StrategyFactory.cpp"
    "${REPO_ROOT}/Y_Manager/MainLoop.cpp"
    "${REPO_ROOT}/Y_Manager/CompressionLoop.cpp"
    "${REPO_ROOT}/Y_Manager/DecompressionLoop.cpp"
    "${REPO_ROOT}/EncryptionModules/Aes/src/AesFunctions.cpp"
    "${REPO_ROOT}/EncryptionModules/Aes/src/mainCircle.cpp"
    "${REPO_ROOT}/BufferPool/BufferPool.cpp"
    "${REPO_ROOT}/CompressionModules/Huffman/Hufftype/HuffmanType.cpp"
    "${REPO_ROOT}/CompressionModules/Huffman/Core/src/Huffman.cpp"
)
target_link_libraries(tool_archivebaseline PRIVATE advapi32)
