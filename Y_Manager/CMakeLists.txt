cmake_minimum_required(VERSION 3.20)

project(SecureCompressor)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_C_COMPILER "C:/msys64/ucrt64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "C:/msys64/ucrt64/bin/g++.exe")
set(WINDRES "C:/msys64/ucrt64/bin/windres.exe")

set(OUTPUT_EXE "${CMAKE_SOURCE_DIR}/bin/Secure Files Compressor.exe")

# ========================
# 资源文件
# ========================
set(RES_OBJ "${CMAKE_BINARY_DIR}/app_res.o")

add_custom_command(
    OUTPUT ${RES_OBJ}
    COMMAND ${WINDRES} -i ${CMAKE_SOURCE_DIR}/app.rc -o ${RES_OBJ}
    DEPENDS ${CMAKE_SOURCE_DIR}/app.rc
)

# ========================
# include 目录
# ========================
include_directories(
    "${CMAKE_SOURCE_DIR}/../EncryptionModules/Aes/include"
    "${CMAKE_SOURCE_DIR}/../DataBlocks"
    "${CMAKE_SOURCE_DIR}/../CompressionModules/hefftype"
    "${CMAKE_SOURCE_DIR}/../CompressionModules/heffman/include"
    "${CMAKE_SOURCE_DIR}/../Schedule"
    "${CMAKE_SOURCE_DIR}/../Schedule/include"
    "${CMAKE_SOURCE_DIR}/../ThreadPool"
    "${CMAKE_SOURCE_DIR}/../CompressorFileSystem/include"
    "${CMAKE_SOURCE_DIR}/../Y_Manager"
)

# ========================
# 源文件
# ========================
set(SOURCES
    "${CMAKE_SOURCE_DIR}/../CompressorFileSystem/DataCommunication/src/EntryParser.cpp"
    "${CMAKE_SOURCE_DIR}/../CompressorFileSystem/DataCommunication/src/ToolClasses.cpp"
    "${CMAKE_SOURCE_DIR}/../CompressorFileSystem/DataCommunication/src/DataExporter.cpp"
    "${CMAKE_SOURCE_DIR}/../CompressorFileSystem/DataCommunication/src/DataLoader.cpp"
    "${CMAKE_SOURCE_DIR}/../CompressorFileSystem/DataCommunication/src/EntryProcessor.cpp"
    "${CMAKE_SOURCE_DIR}/../CompressorFileSystem/DataCommunication/src/BinaryStandardWriter.cpp"
    "${CMAKE_SOURCE_DIR}/../CompressorFileSystem/DataCommunication/src/BinaryStandardLoader.cpp"
    "${CMAKE_SOURCE_DIR}/../Y_Manager/MainLoop.cpp"
    "${CMAKE_SOURCE_DIR}/../CompressorFileSystem/DataCommunication/src/HeaderWriter.cpp"
    "${CMAKE_SOURCE_DIR}/../EncryptionModules/Aes/src/AesFunctions.cpp"
    "${CMAKE_SOURCE_DIR}/../EncryptionModules/Aes/src/mainCircle.cpp"
    "${CMAKE_SOURCE_DIR}/../DataBlocks/DataBlocksManage.cpp"
    "${CMAKE_SOURCE_DIR}/../CompressionModules/hefftype/Heffman_type.cpp"
    "${CMAKE_SOURCE_DIR}/../CompressionModules/heffman/src/DoDecode.cpp"
    "${CMAKE_SOURCE_DIR}/../CompressionModules/heffman/src/DoEncode.cpp"
    "${CMAKE_SOURCE_DIR}/../CompressionModules/heffman/src/GenHeffcodeTab.cpp"
    "${CMAKE_SOURCE_DIR}/../CompressionModules/heffman/src/GetFreq.cpp"
    "${CMAKE_SOURCE_DIR}/../CompressionModules/heffman/src/Heffman.cpp"
    "${CMAKE_SOURCE_DIR}/../CompressionModules/heffman/src/LoadHeffcodeTab.cpp"
    "${CMAKE_SOURCE_DIR}/../Schedule/Scheduler.cpp"
    "${CMAKE_SOURCE_DIR}/../ThreadPool/src/_Thread.cpp"
    "${CMAKE_SOURCE_DIR}/../ThreadPool/src/MonitorTaskQueue.cpp"
    "${CMAKE_SOURCE_DIR}/../ThreadPool/src/ThreadPool.cpp"
    "${CMAKE_SOURCE_DIR}/main.cpp"
    "${CMAKE_SOURCE_DIR}/IconHandler.cpp"
    ${RES_OBJ}
)

add_executable(SecureCompressor ${SOURCES})

# ========================
# 编译 & 链接参数
# ========================
# ========================
# 不同模式的编译配置
# ========================

# Debug（开发用：快 + 可调试）
if(CMAKE_BUILD_TYPE STREQUAL "Debug")

    target_compile_options(SecureCompressor PRIVATE
        -g3
        -Og
        -gdwarf-4

        # 保留结构但不做激进优化
        -ffunction-sections
        -fdata-sections

        # ⚠️ 不开这些（保证调试正常）
        # -flto
        # -fno-exceptions
        # -fvisibility=hidden
    )

    target_link_options(SecureCompressor PRIVATE
        -static-libgcc
        -static-libstdc++
        # ⚠️ 不 strip
    )

endif()


# Release（性能优先）
if(CMAKE_BUILD_TYPE STREQUAL "Release")

    target_compile_options(SecureCompressor PRIVATE
        -O3
        -DNDEBUG
        -ffunction-sections
        -fdata-sections
        -fvisibility=hidden
        -fno-rtti
    )

    target_link_options(SecureCompressor PRIVATE
        -Wl,--gc-sections
        -Wl,--as-needed
        -static
        -static-libgcc
        -static-libstdc++
        -s
    )

endif()


# MinSize（极限体积版 👉 你现在主力）
if(CMAKE_BUILD_TYPE STREQUAL "MinSize")

    target_compile_options(SecureCompressor PRIVATE
        -Os
        -DNDEBUG
        -ffunction-sections
        -fdata-sections
        -fvisibility=hidden
        -fno-rtti

    )

    target_link_options(SecureCompressor PRIVATE
        -Wl,--gc-sections
        -Wl,--as-needed
        -static
        -static-libgcc
        -static-libstdc++
        -s
    )

endif()

# ========================
# 链接库
# ========================
target_link_libraries(SecureCompressor
    ws2_32
    crypt32
    gdi32
)

# ========================
# 输出路径
# ========================
set_target_properties(SecureCompressor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    OUTPUT_NAME "Secure Files Compressor"
)
