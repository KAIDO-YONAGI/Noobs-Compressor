cmake_minimum_required(VERSION 3.20)

project(SecureCompressorGUI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ========================
# Qt 路径配置（使用 Qt 自带的 MinGW 编译器）
# ========================
set(CMAKE_PREFIX_PATH "D:/qt/6.10.1/mingw_64/lib/cmake/Qt6")
set(CMAKE_C_COMPILER "D:/qt/Tools/mingw1310_64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "D:/qt/Tools/mingw1310_64/bin/g++.exe")

# ========================
# 资源文件
# ========================
set(RESOURCES
    "${CMAKE_SOURCE_DIR}/resources.qrc"
)

# ========================
# Windows资源
# ========================
if (WIN32)
    enable_language(RC)
    set(WINRES_OBJ "${CMAKE_BINARY_DIR}/app_res.o")
    add_custom_command(
        OUTPUT ${WINRES_OBJ}
        COMMAND windres -i "${CMAKE_SOURCE_DIR}/app.rc" -o ${WINRES_OBJ}
        DEPENDS "${CMAKE_SOURCE_DIR}/app.rc" "${CMAKE_SOURCE_DIR}/YONAGII_512x512.ico"
    )
endif()

# ========================
# 查找 Qt 包
# ========================
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

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
# 源文件（核心库）
# ========================
set(CORE_SOURCES
    "${CMAKE_SOURCE_DIR}/../Y_Manager/IconHandler.cpp"
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
)

# ========================
# GUI 源文件
# ========================
set(GUI_SOURCES
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_SOURCE_DIR}/src/MainWindow.cpp"
    "${CMAKE_SOURCE_DIR}/src/CompressionWorker.cpp"
)

# ========================
# 创建可执行文件
# ========================
add_executable(SecureCompressorGUI
    ${CORE_SOURCES}
    ${GUI_SOURCES}
    ${RESOURCES}
)

# ========================
# 添加Windows资源对象
# ========================
if (WIN32)
    target_sources(SecureCompressorGUI PRIVATE ${WINRES_OBJ})
endif()

# ========================
# 链接库
# ========================
target_link_libraries(SecureCompressorGUI PRIVATE
    Qt6::Core
    Qt6::Widgets
)

# ========================
# Windows 系统库
# ========================
if (WIN32)
    # Release 模式下隐藏控制台窗口
    set_target_properties(SecureCompressorGUI PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_link_libraries(SecureCompressorGUI PRIVATE
        ws2_32
        crypt32
        gdi32
        shell32
        ole32
    )
endif()

# ========================
# 编译选项
# ========================
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    if (NOT MSVC)
        target_compile_options(SecureCompressorGUI PRIVATE
            -O3
            -DNDEBUG
            -ffunction-sections
            -fdata-sections
        )
        target_link_options(SecureCompressorGUI PRIVATE
            -Wl,--gc-sections
            -Wl,--as-needed
            -static-libgcc
            -static-libstdc++
            -s
        )
    endif()
endif()

# ========================
# 输出路径
# ========================
set_target_properties(SecureCompressorGUI PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    OUTPUT_NAME "SecureCompressorGUI"
)
