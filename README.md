
# SFC.exe (Windows Only)

---

# 项目简介 | Project Overview

**SFC.exe** 是一个基于 **Huffman 编码与 AES 加密**实现的文件归档工具，支持对**多个文件与目录进行压缩与解压缩**。  
**SFC.exe** is a file archiving tool based on **Huffman coding and AES encryption**, supporting **compression and decompression of multiple files and directories**.

该项目主要用于研究**文件系统归档结构、数据压缩以及基础加密实现**。  
The project is mainly intended for studying **file archiving structures, data compression, and basic encryption implementations**.

**当前提供两个版本**：
- **命令行版本（CLI）**：`Y_Manager/` 目录，v1.x.x
- **图形界面版本（GUI）**：`Y_Manager_GUI/` 目录，v2.x.x

**Two versions are available**:
- **Command-line version (CLI)**: `Y_Manager/` directory, v1.x.x
- **GUI version**: `Y_Manager_GUI/` directory, v2.x.x

---

# 功能特性 | Features

- 支持**多个文件与目录的归档压缩与解压缩**  
  Supports **archiving, compression, and decompression of multiple files and directories**

- 使用 **Huffman 编码**实现数据压缩  
  Uses **Huffman coding** for data compression

- 支持 **AES 加密**与 **SHA256** 相关安全功能  
  Supports **AES encryption** and **SHA256** related security functions

- 采用 **分块压缩 / 解压机制**，降低内存占用  
  Uses a **block-based compression and decompression mechanism** to reduce memory usage

- 引入 **逻辑根目录（Logical Root）结构**以统一归档路径  
  Introduces a **logical root directory structure** to manage archive paths

- 实现 **路径重复跳过机制**以避免部分重复文件处理  
  Implements a **duplicate-path skipping mechanism** to avoid redundant processing

- **GUI 版本特性**：拖放支持、实时进度、日志输出  
  **GUI features**: drag-and-drop support, real-time progress, log output

---

# 性能说明 | Performance Notes

由于 **Huffman 编码与 AES 加密均为纯软件实现**，程序运行速度相对较慢。  
Since **Huffman coding and AES encryption are implemented purely in software**, runtime performance is relatively slow.

压缩算法仅依赖 Huffman，因此**压缩率有限**，最佳情况下约 **60%**。  
Because compression relies solely on Huffman coding, the **compression ratio is limited**, with a best-case ratio of about **60%**.

程序采用**分块处理策略**，在路径长度正常的情况下：  
The program uses a **block-processing strategy**, and under normal path length conditions:

- **内存峰值约为 64MB**  
  **Peak memory usage is approximately 64 MB**

测试环境：  
Test scenario:

- 约 **13,000 个文件与目录**  
  Approximately **13,000 files and directories**

- 总数据量约 **230GB**  
  Total data size around **230 GB**

---

# 安全提示 | Safety Notice

虽然程序已经实现：

- **逻辑根目录机制**
- **路径重复跳过机制**

但在极端情况下仍**可能发生文件覆盖**。

Although the program includes:

- a **logical root directory mechanism**
- a **duplicate-path skipping mechanism**

file overwriting **may still occur in extreme situations**.

在使用软件前，请务必**备份原始文件**。  
Please **always back up your original files before using the software**.

---

# 项目文档 | Project Documentation

项目相关文档包括：

The project documentation includes:

- `instructions.md`  
  项目说明与 HuffmanZip 相关设计文档  
  Project instructions and HuffmanZip design documentation

- `策划.md`  
  项目设计规划文档  
  Project planning document

- `开发日志.md`  
  开发时间线与技术细节  
  Development timeline and technical details

---

# 版本历史 | Version History

## v1.0.0 — Preview

存在少量已知 Bug。  
A small number of known bugs existed.

主要原因是 **Huffman 树在处理单字符输入时构建不正确**。  
The main cause was **incorrect construction of the Huffman tree when handling single-character input**.

---

## v1.0.1 — Stable

修复 Huffman 编码边界处理问题。  
Fixed boundary handling issues in Huffman encoding.

对 EXE 文件进行了轻量化优化与外观调整。  
Applied lightweight optimization and visual adjustments to the executable.

编译优化等级从 **O2 提升到 O3**。  
The compiler optimization level was upgraded from **O2 to O3**.

---

## v1.1.1 — Well Done (CLI)

封装了文件 I/O 以及若干辅助方法（例如 `seek*` 系列函数）。  
Encapsulated file I/O operations and several helper methods (such as `seek*` functions).

修复了文件目录分块处理问题。  
Fixed the directory block processing issue.

内存占用稳定在 **60MB 左右**。  
Memory usage is stable around **60 MB**.

---

## v2.0.0 — GUI Release (2026-04-16)

**新增功能**：
- Qt 6 图形用户界面
- 左右两列布局：左列输入配置，右列进度输出
- 支持拖放文件/目录
- 实时进度显示和日志输出
- 精简部署（减少约 30MB）
- UPX 压缩（减少 40-60%）

**New Features**:
- Qt 6 graphical user interface
- Left-right two-column layout
- Drag-and-drop support
- Real-time progress and log output
- Minimal deployment (reduced by ~30MB)
- UPX compression (reduced by 40-60%)

---

# 构建指南 | Build Instructions

## CLI 版本构建 | CLI Build

如需自行编译命令行版本：

To compile the CLI version yourself:

### 1 下载构建配置 | Download Build Configuration

从 Release 页面下载 `.vscode` 压缩包。  
Download the `.vscode` archive from the Release page.

---

### 2 放入项目目录 | Place in Project Directory

将解压后的 `.vscode` 文件夹放入：

Place the extracted `.vscode` folder into:

```
Y_Manager/
```

---

### 3 编译程序 | Compile the Program

编译 `main.cpp` 文件即可生成可执行程序。  
Compile `main.cpp` to generate the executable.

---

## GUI 版本构建 | GUI Build

GUI 版本位于 `Y_Manager_GUI/` 目录，需要 Qt 6 环境。

The GUI version is in `Y_Manager_GUI/` directory, requires Qt 6.

### 编译要求 | Requirements

- **Qt 6.10.1**（使用自带的 MinGW 13.1.0）
- **CMake 3.20+**
- **UPX**（可选，用于压缩）

### 构建步骤 | Build Steps

```bash
cd Y_Manager_GUI
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 8
```

构建完成后，可执行文件位于 `bin/SFC/` 目录。

After building, the executable is in `bin/SFC/` directory.

---

### 编译要求 | Compilation Requirements

- 使用 **C++20 标准**  
  Use the **C++20 standard**

- 编译器选项：  
  Compiler option:

```
-std=c++20
```

- 推荐优化等级：  
  Recommended optimization level:

```
-O3
```

- **不要启用 LTO（链接时优化）**  
  **Do not enable LTO (Link Time Optimization)**

- **GUI 版本必须使用 Qt 自带的 MinGW**  
  **GUI version must use Qt's bundled MinGW**

---

# 依赖 | Dependencies

## CLI 版本 | CLI Dependencies

- **OpenSSL**
  - `SHA256`
  - `RAND`

- **Windows 10–11 API**
  用于字符编码控制以及随机数生成
  Used for character encoding control and rand() 

## GUI 版本 | GUI Dependencies

- **Qt 6.10.1** (Core, Widgets)
- **MinGW 13.1.0** (Qt bundled)
- **Windows 10–11 API**

---

# 免责声明 | Disclaimer

本项目仅用于 **教学与学习目的**。  
This project is intended **for educational and learning purposes only**.

使用前请务必 **备份原始文件**。  
Please **back up your original files before using the software**.

