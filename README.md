
# SFC.exe (Windows Only)

---

# 项目简介 | Project Overview

**SFC.exe** 是一个基于 **Huffman 编码与 AES 加密**实现的文件归档工具，支持对**多个文件与目录进行压缩与解压缩**。  
**SFC.exe** is a file archiving tool based on **Huffman coding and AES encryption**, supporting **compression and decompression of multiple files and directories**.

该项目主要用于研究**文件系统归档结构、数据压缩以及基础加密实现**。  
The project is mainly intended for studying **file archiving structures, data compression, and basic encryption implementations**.

目前程序仅提供**命令行交互（CLI）**，尚未实现图形界面（GUI）。  
The program currently provides only a **command-line interface (CLI)** and does not include a graphical user interface (GUI).

更多系统实现细节可参考源代码及相关 Markdown 文档。  
For more implementation details, please refer to the source code and the related Markdown documents.

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

- `plan.md`  
  AES 与压缩文件系统设计说明  
  AES and compressed file system design documentation

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

仍可能存在 **未知 Bug**，在部分情况下可能导致 **文件数据解析失败**。  
Some **unknown bugs** may still exist, which could cause **file data parsing failures** in certain cases.

---

## v1.0.1 — Well Done

封装了文件 I/O 以及若干辅助方法（例如 `seek*` 系列函数）。  
Encapsulated file I/O operations and several helper methods (such as `seek*` functions).

修复了文件目录分块处理问题。  
Fixed the directory block processing issue.

理论上，在程序正常运行的情况下，压缩任意文件或目录时 **内存占用可稳定在 60MB 左右**。  
In theory, as long as the program runs normally, compressing any file or directory should keep **memory usage stable around 60 MB**.

需要注意的是：  
Note that:

编译时启用 **LTO（Link Time Optimization）** 等高级优化可能导致压缩流程异常。  
Enabling **LTO (Link Time Optimization)** or similar advanced optimizations during compilation may cause errors in the compression process.

因此建议 **仅使用 O3 优化等级进行编译**。  
Therefore, it is recommended to **compile using only the O3 optimization level**.

当前版本 **没有已知 Bug**。  
There are **currently no known bugs**.

---

# 构建指南 | Build Instructions

如需自行编译程序：

To compile the program yourself:

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

- 某些编译器需要链接 **stdc++fs** 以支持文件系统功能  
  Some compilers require linking **stdc++fs** to support filesystem features

- **不要启用 LTO（链接时优化）**  
  **Do not enable LTO (Link Time Optimization)**

---

# 依赖 | Dependencies

- **Windows 10–11 API**

  用于字符编码控制以及随机数生成
  Used for character encoding control and rand() 

---

# 免责声明 | Disclaimer

本项目仅用于 **教学与学习目的**。  
This project is intended **for educational and learning purposes only**.

使用前请务必 **备份原始文件**。  
Please **back up your original files before using the software**.



