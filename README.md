# SFC.exe (Windows Only)

> 数据结构与算法课程大作业｜华南师范大学  
> Final Project for *Data Structures and Algorithms* — South China Normal University

---

## 项目说明 | Project Description

**本项目作为华南师范大学《数据结构与算法》课程大作业**  
**项目第一贡献者，学号：202440025284**

**This project is developed as the final assignment for the course _Data Structures and Algorithms_ at South China Normal University.**  
**First contributor: Student ID 202440025284**

---

## SFC.exe（仅支持 Windows） | SFC.exe (Windows Only)

- 程序具备文件归档系统功能，支持**多个文件与目录的压缩与解压缩**。  
  具体系统架构请参见源代码及相关 Markdown 文档。

- The program implements a **file archiving system**, supporting **compression and decompression of multiple files and directories**.  
  For detailed architecture, please refer to the source code and related Markdown documents.

- 目前仅提供**基础的命令行交互（CLI）**，暂无 GUI。  
- The program currently provides only **basic command-line interaction (CLI)**, with **no GUI support**.

- 如需自行编译可执行文件，可在 `Y_Manager` 目录下找到 `main.cpp` 进行编译  
  （可从 Release 下载 `.vscode` 压缩包获取配置）。

- To compile the executable yourself, locate `main.cpp` in the `Y_Manager` directory.  
  (You may download the `.vscode` archive from the Release page to obtain build configurations.)

- 由于 **Huffman 编码与 AES 加密均为纯软件实现**，运行速度相对较慢；  
  压缩算法仅依赖 Huffman，因此**压缩率有限**（最佳约 **60%**）。

- Since **Huffman encoding and AES encryption are implemented purely in software**, performance is relatively slow.  
  As compression relies solely on Huffman coding, the **compression ratio is limited** (best case ~**60%**).

- 虽然已加入**逻辑根目录**与**路径重复跳过机制**，  
  但极端情况下仍**可能覆盖原文件**，请务必提前备份数据。

- Although a **logical root directory** and **duplicate-path skipping mechanism** are implemented,  
  **file overwriting may still occur in rare cases**. Please back up your original files.

- 软件采用**分块压缩 / 解压缩**策略，  
  在路径长度正常的情况下，**内存峰值约 64MB**。  
  测试场景：约 **13K 个目录及文件**，总数据量 **230GB**。

- The software uses **block-based compression and decompression**.  
  Under normal conditions, **peak memory usage is approximately 64MB**.  
  Test scenario: ~**13K directories and files**, total size **230GB**.

---

## 项目文档 | Project Documentation

- `instructions.md` & `策划.md` — Instructions & HuffmanZip  
- `plan.md` — AES & Compressor File System

---

## 版本更新 | Version History

### v1.0.0 — Preview
- 存在少量已知 Bug，经排查发现主要原因是 **Huffman 树在处理单字符输入时构建不当**
- Contains a small number of known bugs, mainly caused by **incorrect Huffman tree construction when handling single-character inputs**

### v1.0.1 — Stable
- 修复 Huffman 编码的边界处理问题
- Fixed boundary handling issues in Huffman encoding
- 对 EXE 文件进行了轻量化与外观优化
- Applied lightweight optimization and visual refinement to the executable
- 编译优化等级：`O2 → O3`
- Compiler optimization level upgraded from `O2` to `O3`
- 目前仍发现存在 **未知 Bug**，在部分情况下可能导致 **文件数据解析失败**
- Some **unknown bugs** are still present, which may cause **file data parsing failures** in certain cases


---

## 构建说明 | Build Instructions

1. 从 Release 页面下载 `.vscode` 压缩包  
2. 解压并放入 `Y_Manager/` 目录  
3. 配置依赖并编译 `main.cpp`

### 依赖 | Dependencies

- **OpenSSL**
  - `SHA256`
  - `RAND`
- Windows 10–11 API（字符编码控制）

---

## 免责声明 | Disclaimer

本项目仅用于**教学与学习目的**。  
使用前请务必**备份原始文件**。

This project is intended **for educational purposes only**.  
Always **back up your original files** before use.
