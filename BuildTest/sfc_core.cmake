# sfc_core.cmake —— 核心库源文件清单与包含目录的唯一定义
#
# 被两处 include：
#   BuildTest/CMakeLists.txt         （GUI 可执行文件）
#   BuildTest/tests/CMakeLists.txt   （单元测试 + tool_archivebaseline）
#
# 此前两边各抄一份 21 文件清单与 9 条 -I（内容相同、顺序不同），加 .cpp 时
# 漏改一处就会出现"GUI 编得过、测试链不上"的假绿。此处收敛为单一来源。
#
# 刻意不建 static library：静态库会把 LTO 的作用域切到库边界，跨 TU 内联失效。
# 本工程实测的提速有相当一部分正来自跨 TU 内联，因此这里共享的是"清单"而非"目标"。
# 要共享的从来是同一份列表，不是一个编译单元集合。
#
# 用法（在调用方 CMakeLists 中、add_executable 之前）：
#   GUI   ：include("${CMAKE_CURRENT_LIST_DIR}/sfc_core.cmake")
#   tests ：include("${CMAKE_CURRENT_LIST_DIR}/../sfc_core.cmake")
#   include_directories(${SFC_CORE_INCLUDE_DIRS})
#   add_executable(<target> ${SFC_CORE_SOURCES} ...)

# 本文件位于 <repo>/BuildTest/，故其上一级即仓库根
set(SFC_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

set(SFC_CORE_INCLUDE_DIRS
    "${SFC_REPO_ROOT}/EncryptionModules/Aes/include"
    "${SFC_REPO_ROOT}/CompressionModules/Huffman/Hufftype"
    "${SFC_REPO_ROOT}/CompressionModules/Huffman/Core/include"
    "${SFC_REPO_ROOT}/BufferPool"
    "${SFC_REPO_ROOT}/CompressorFileSystem/Commons/include"
    "${SFC_REPO_ROOT}/CompressorFileSystem/DataIO/include"
    "${SFC_REPO_ROOT}/CompressorFileSystem/ArchiveFormat/include"
    "${SFC_REPO_ROOT}/CompressorFileSystem/Strategy/include"
    "${SFC_REPO_ROOT}/Y_Manager"
)

set(SFC_CORE_SOURCES
    "${SFC_REPO_ROOT}/CompressorFileSystem/Commons/src/EncodingUtils.cpp"
    "${SFC_REPO_ROOT}/CompressorFileSystem/Commons/src/FileSystemUtils.cpp"
    "${SFC_REPO_ROOT}/CompressorFileSystem/Commons/src/ToolClasses.cpp"
    "${SFC_REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/EntryParser.cpp"
    "${SFC_REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/EntryProcessor.cpp"
    "${SFC_REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/BinaryStandardWriter.cpp"
    "${SFC_REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/BinaryStandardLoader.cpp"
    "${SFC_REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/CatalogFinalizer.cpp"
    "${SFC_REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/HeaderWriter.cpp"
    "${SFC_REPO_ROOT}/CompressorFileSystem/DataIO/src/DataExporter.cpp"
    "${SFC_REPO_ROOT}/CompressorFileSystem/DataIO/src/DataLoader.cpp"
    "${SFC_REPO_ROOT}/CompressorFileSystem/Strategy/src/NullCompression.cpp"
    "${SFC_REPO_ROOT}/CompressorFileSystem/Strategy/src/StrategyFactory.cpp"
    "${SFC_REPO_ROOT}/EncryptionModules/Aes/src/AesFunctions.cpp"
    "${SFC_REPO_ROOT}/EncryptionModules/Aes/src/mainCircle.cpp"
    "${SFC_REPO_ROOT}/CompressionModules/Huffman/Hufftype/HuffmanType.cpp"
    "${SFC_REPO_ROOT}/CompressionModules/Huffman/Core/src/Huffman.cpp"
    "${SFC_REPO_ROOT}/BufferPool/BufferPool.cpp"
    "${SFC_REPO_ROOT}/Y_Manager/MainLoop.cpp"
    "${SFC_REPO_ROOT}/Y_Manager/CompressionLoop.cpp"
    "${SFC_REPO_ROOT}/Y_Manager/DecompressionLoop.cpp"
)

# 单一来源的即时校验：清单里写错路径时在配置阶段就报文件级错误，
# 而不是等链接期出现一堆 undefined reference 再回头找
foreach(_sfc_src IN LISTS SFC_CORE_SOURCES)
    if(NOT EXISTS "${_sfc_src}")
        message(FATAL_ERROR "sfc_core.cmake: 源文件不存在 -> ${_sfc_src}")
    endif()
endforeach()
foreach(_sfc_inc IN LISTS SFC_CORE_INCLUDE_DIRS)
    if(NOT IS_DIRECTORY "${_sfc_inc}")
        message(FATAL_ERROR "sfc_core.cmake: 包含目录不存在 -> ${_sfc_inc}")
    endif()
endforeach()
unset(_sfc_src)
unset(_sfc_inc)
