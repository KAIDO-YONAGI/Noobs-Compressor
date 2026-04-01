cmake_minimum_required(VERSION 3.20)

project(SecureCompressor)

# ========================
# C++ 标准（通用写法）
# ========================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========================
# 不要在这里写编译器路径 ❗
# ========================

# ========================
# 资源文件（Windows）
# ========================
if (WIN32)
    enable_language(RC)

    set(RES_OBJ "${CMAKE_BINARY_DIR}/app_res.o")

    add_custom_command(
        OUTPUT ${RES_OBJ}
        COMMAND windres -i ${CMAKE_SOURCE_DIR}/app.rc -o ${RES_OBJ}
        DEPENDS ${CMAKE_SOURCE_DIR}/app.rc
    )
endif()

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
)

if (WIN32)
    list(APPEND SOURCES ${RES_OBJ})
endif()

add_executable(SecureCompressor ${SOURCES})

# ========================
# 编译选项（核心通用逻辑）
# ========================

# Debug
if (CMAKE_BUILD_TYPE STREQUAL "Debug")

    if (MSVC)
        target_compile_options(SecureCompressor PRIVATE
            /Zi
            /Od
        )
    else()
        target_compile_options(SecureCompressor PRIVATE
            -g3
            -Og
            -gdwarf-4
            -ffunction-sections
            -fdata-sections
        )
    endif()

endif()

# Release
if (CMAKE_BUILD_TYPE STREQUAL "Release")

    if (MSVC)
        target_compile_options(SecureCompressor PRIVATE
            /O2
            /DNDEBUG
        )
    else()
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
            -static-libgcc
            -static-libstdc++
            -s
        )
    endif()

endif()

# MinSize
if (CMAKE_BUILD_TYPE STREQUAL "MinSize")

    if (MSVC)
        target_compile_options(SecureCompressor PRIVATE
            /O1
            /DNDEBUG
        )
    else()
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
            -static-libgcc
            -static-libstdc++
            -s
        )
    endif()

endif()

# ========================
# Windows 系统库
# ========================
if (WIN32)
    target_link_libraries(SecureCompressor
        ws2_32
        crypt32
        gdi32
    )
endif()

# ========================
# 输出路径
# ========================
set_target_properties(SecureCompressor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    OUTPUT_NAME "Secure Files Compressor"
)