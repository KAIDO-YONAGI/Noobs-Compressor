cmake_minimum_required(VERSION 3.20)

project(SimpleFilesCompressorGUI VERSION 2.1.0 LANGUAGES CXX)
# 构建与测试位于根目录 BuildTest/（本文件为 GUI 构建，tests/ 为单元测试）：GUI 代码在 ../Y_Manager_GUI，其余模块在仓库根各目录
set(REPO_ROOT "${CMAKE_SOURCE_DIR}/..")
set(GUI_DIR "${CMAKE_SOURCE_DIR}/../Y_Manager_GUI")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ========================
# Qt 路径配置（Qt 6.2.4 LTS - 兼容 Windows 10 1607+）
# ========================
set(USE_STATIC_QT ON CACHE BOOL "Use statically-linked Qt build")
if(USE_STATIC_QT)
    set(CMAKE_PREFIX_PATH "D:/qt/6.2.4-static-mingw")
else()
    set(CMAKE_PREFIX_PATH "D:/qt/6.2.4/mingw_64")
endif()
set(CMAKE_C_COMPILER "D:/qt/Tools/mingw1120_64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "D:/qt/Tools/mingw1120_64/bin/g++.exe")

# ========================
# 资源文件
# ========================
set(RESOURCES
    "${GUI_DIR}/resources.qrc"
)

set(APP_TRANSLATION_FILES
    "${GUI_DIR}/translations/SimpleFilesCompressorGUI_zh_CN.qm"
)

option(SFC_COPY_EXTERNAL_TRANSLATIONS "Copy replaceable translation files next to the executable" OFF)

# ========================
# Windows资源
# ========================
if (WIN32)
    enable_language(RC)
    set(WINRES_OBJ "${CMAKE_BINARY_DIR}/app_res.o")
    add_custom_command(
        OUTPUT ${WINRES_OBJ}
        COMMAND windres -i "${GUI_DIR}/app.rc" -o ${WINRES_OBJ}
        DEPENDS "${GUI_DIR}/app.rc" "${GUI_DIR}/YONAGII_512x512.ico"
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
    "${REPO_ROOT}/EncryptionModules/Aes/include"
    "${REPO_ROOT}/CompressionModules/Huffman/Hufftype"
    "${REPO_ROOT}/CompressionModules/Huffman/Core/include"
    "${REPO_ROOT}/BufferPool"
    "${REPO_ROOT}/CompressorFileSystem/Commons/include"
    "${REPO_ROOT}/CompressorFileSystem/DataIO/include"
    "${REPO_ROOT}/CompressorFileSystem/ArchiveFormat/include"
    "${REPO_ROOT}/CompressorFileSystem/Strategy/include"
    "${REPO_ROOT}/Y_Manager"
)

# ========================
# 源文件（核心库）
# ========================
set(CORE_SOURCES
    "${REPO_ROOT}/CompressorFileSystem/Commons/src/EncodingUtils.cpp"
    "${REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/EntryParser.cpp"
    "${REPO_ROOT}/CompressorFileSystem/Commons/src/ToolClasses.cpp"
    "${REPO_ROOT}/CompressorFileSystem/DataIO/src/DataExporter.cpp"
    "${REPO_ROOT}/CompressorFileSystem/DataIO/src/DataLoader.cpp"
    "${REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/EntryProcessor.cpp"
    "${REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/BinaryStandardWriter.cpp"
    "${REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/BinaryStandardLoader.cpp"
    "${REPO_ROOT}/CompressorFileSystem/Strategy/src/NullCompression.cpp"
    "${REPO_ROOT}/CompressorFileSystem/Strategy/src/StrategyFactory.cpp"
    "${REPO_ROOT}/Y_Manager/MainLoop.cpp"
    "${REPO_ROOT}/Y_Manager/CompressionLoop.cpp"
    "${REPO_ROOT}/Y_Manager/DecompressionLoop.cpp"
    "${REPO_ROOT}/CompressorFileSystem/ArchiveFormat/src/HeaderWriter.cpp"
    "${REPO_ROOT}/EncryptionModules/Aes/src/AesFunctions.cpp"
    "${REPO_ROOT}/EncryptionModules/Aes/src/mainCircle.cpp"
    "${REPO_ROOT}/BufferPool/BufferPool.cpp"
    "${REPO_ROOT}/CompressionModules/Huffman/Hufftype/HuffmanType.cpp"
    "${REPO_ROOT}/CompressionModules/Huffman/Core/src/Huffman.cpp"
)

# ========================
# GUI 源文件
# ========================
set(GUI_SOURCES
    "${GUI_DIR}/src/main.cpp"
    "${GUI_DIR}/src/MainWindow.cpp"
    "${GUI_DIR}/src/MainWindow_Compression.cpp"
    "${GUI_DIR}/src/MainWindow_Decompression.cpp"
    "${GUI_DIR}/src/CompressionWorker.cpp"
    "${GUI_DIR}/IconHandler.cpp"
)

# ========================
# 创建可执行文件
# ========================
add_executable(SimpleFilesCompressorGUI
    ${CORE_SOURCES}
    ${GUI_SOURCES}
    ${RESOURCES}
)

# ========================
# 添加Windows资源对象
# ========================
if (WIN32)
    target_sources(SimpleFilesCompressorGUI PRIVATE ${WINRES_OBJ})
endif()

# ========================
# 链接库
# ========================
target_link_libraries(SimpleFilesCompressorGUI PRIVATE
    Qt6::Core
    Qt6::Widgets
)

# ========================
# Windows 系统库
# ========================
if (WIN32)
    set_target_properties(SimpleFilesCompressorGUI PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_link_libraries(SimpleFilesCompressorGUI PRIVATE
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
        target_compile_options(SimpleFilesCompressorGUI PRIVATE
            -O3
            -DNDEBUG
            -ffunction-sections
            -fdata-sections
        )
        target_link_options(SimpleFilesCompressorGUI PRIVATE
            -Wl,--gc-sections
            -Wl,--as-needed
            -static-libgcc
            -static-libstdc++
            -static
            -s
        )
    endif()

    # 静态 Qt 额外配置
    if(USE_STATIC_QT)
        target_compile_definitions(SimpleFilesCompressorGUI PRIVATE
            QT_STATICPLUGIN
        )

        # 直接链接静态插件 .a 文件
        set(QT_STATIC_PLUGINS
            "D:/qt/6.2.4-static-mingw/plugins/platforms/libqwindows.a"
            "D:/qt/6.2.4-static-mingw/plugins/imageformats/libqjpeg.a"
            "D:/qt/6.2.4-static-mingw/plugins/imageformats/libqico.a"
            "D:/qt/6.2.4-static-mingw/plugins/imageformats/libqgif.a"
            "D:/qt/6.2.4-static-mingw/plugins/styles/libqwindowsvistastyle.a"
        )

        target_link_libraries(SimpleFilesCompressorGUI PRIVATE
            ${QT_STATIC_PLUGINS}
            dwmapi
            uxtheme
            imm32
            oleaut32
            version
            setupapi
            fontsub
        )
    endif()
endif()

# ========================
# 输出路径
# ========================
set_target_properties(SimpleFilesCompressorGUI PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/SFC"
    OUTPUT_NAME "SimpleFilesCompressorGUI"
)

if(SFC_COPY_EXTERNAL_TRANSLATIONS)
    foreach(QM_FILE ${APP_TRANSLATION_FILES})
        get_filename_component(QM_FILE_NAME "${QM_FILE}" NAME)
        add_custom_command(TARGET SimpleFilesCompressorGUI POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/bin/SFC/translations"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QM_FILE}"
                "${CMAKE_SOURCE_DIR}/bin/SFC/translations/${QM_FILE_NAME}"
            COMMENT "Copying UI translation: ${QM_FILE_NAME}"
        )
    endforeach()
endif()

# ========================
# 精简部署 - 仅动态链接模式
# ========================
if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT USE_STATIC_QT)
    # 定义要删除的不必要文件列表
    # 注意：MinGW 运行时 DLL 需要保留，因为 Qt DLL 动态依赖它们
    set(UNNECESSARY_DLLS
        "Qt6Network.dll"
        "Qt6Svg.dll"
        "D3Dcompiler_47.dll"
        # "opengl32sw.dll"      # 保留：无 GPU 系统的软件渲染后备
    )
    set(UNNECESSARY_DIRS
        "iconengines"
        "networkinformation"
        "tls"
        # "generic"             # 保留：平台插件，可能需要
        "styles"
    )
    set(UNNECESSARY_IMAGEFORMATS
        "imageformats/qsvg.dll"
    )

    # 创建部署命令
    add_custom_command(TARGET SimpleFilesCompressorGUI POST_BUILD
        COMMAND "D:/qt/6.2.4/mingw_64/bin/windeployqt.exe"
                "$<TARGET_FILE:SimpleFilesCompressorGUI>"
                --release
                --no-translations
        COMMENT "Deploying Qt dependencies..."
    )

    # 复制 MinGW 运行时 DLL（Qt DLL 依赖它们）
    set(MINGW_RUNTIME_DLLS
        "libgcc_s_seh-1.dll"
        "libstdc++-6.dll"
        "libwinpthread-1.dll"
    )
    foreach(DLL ${MINGW_RUNTIME_DLLS})
        add_custom_command(TARGET SimpleFilesCompressorGUI POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "D:/qt/Tools/mingw1120_64/bin/${DLL}"
                "${CMAKE_SOURCE_DIR}/bin/SFC/${DLL}"
            COMMENT "Copying MinGW runtime: ${DLL}"
        )
    endforeach()

    # 删除不必要的 DLL
    foreach(DLL ${UNNECESSARY_DLLS})
        add_custom_command(TARGET SimpleFilesCompressorGUI POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_SOURCE_DIR}/bin/SFC/${DLL}"
            COMMENT "Removing unnecessary: ${DLL}"
        )
    endforeach()

    # 删除不必要的目录
    foreach(DIR ${UNNECESSARY_DIRS})
        add_custom_command(TARGET SimpleFilesCompressorGUI POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_SOURCE_DIR}/bin/SFC/${DIR}"
            COMMENT "Removing unnecessary directory: ${DIR}"
        )
    endforeach()

    # 删除不必要的图像格式插件
    foreach(PLUGIN ${UNNECESSARY_IMAGEFORMATS})
        add_custom_command(TARGET SimpleFilesCompressorGUI POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_SOURCE_DIR}/bin/SFC/${PLUGIN}"
            COMMENT "Removing unnecessary plugin: ${PLUGIN}"
        )
    endforeach()

    # 显示部署完成信息
    add_custom_command(TARGET SimpleFilesCompressorGUI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "=== Minimal deployment complete ==="
        COMMENT "Deployment finished"
    )
endif()
