---

## 项目说明 | Project Description

**本项目作为华南师范大学《数据结构与算法》课程大作业**
**项目第一贡献者，学号：202440025284**

**This project is developed as the final assignment for the course *Data Structures and Algorithms* at South China Normal University.**
**First contributor: Student ID 202440025284**

---

## SFC.exe（仅支持 Windows）

## SFC.exe (Windows Only)

* 程序具备文件归档系统功能，支持**多个文件与目录的压缩与解压缩**。
  具体系统架构请参见源代码及相关 Markdown 文档。

* The program implements a **file archiving system**, supporting **compression and decompression of multiple files and directories**.
  For detailed architecture, please refer to the source code and related Markdown documents.

* 程序目前仅提供**基础的命令行交互（CLI）**，暂无 GUI。

* The program currently provides only **basic command-line interaction (CLI)**, with **no GUI support**.

* 如需自行编译可执行文件，可在 `Y_Manager` 目录下找到 `main.cpp` 进行编译
  （可从 Release 中下载 `.vscode` 压缩包以获取编译配置）。

* To compile the executable yourself, locate `main.cpp` in the `Y_Manager` directory.
  (You may download the `.vscode` archive from the Release page to obtain build configurations.)

* 由于 **Huffman 编码与 AES 加密均采用纯软件实现**，运行速度相对较慢；
  且压缩算法仅依赖 Huffman 编码，因此**压缩率有限**（最佳情况下约 **60%**）。

* Since **Huffman encoding and AES encryption are implemented purely in software**, performance is relatively slow.
  Additionally, as compression relies solely on Huffman coding, the **compression ratio is limited** (best case ~**60%**).

* 虽然已添加**逻辑根目录**与**路径重复检测与跳过机制**，
  但在极端情况下仍**可能覆盖原始文件**，请务必谨慎操作并**提前备份重要数据**。

* Although a **logical root directory** and **duplicate-path skipping mechanism** are implemented,
  **file overwriting may still occur in rare cases**. Please proceed with caution and **back up your original files**.

* 软件采用**分块压缩与解压缩**策略，在文件名及路径长度正常的情况下，
  **内存占用峰值约为 64MB**。
  测试场景：约 **13K 个目录及文件**、目录深度正常、总数据量 **230GB**。

* The software uses **block-based compression and decompression**.
  Under normal filename and path-length conditions, the **peak memory usage is approximately 64MB**.
  Test scenario: ~**13K directories and files**, normal directory depth, total size **230GB**.

---

## 项目文档说明 | Project Documentation

* `instructions.md` & `策划.md` —— Instructions & HuffmanZip
* `plan.md` —— AES & Compressor File System

---

## 版本更新记录 | Version History

### v1.0.0 — 预览版 | Preview Version

* 存在少量已知 Bug
* Contains a small number of known bugs

### v1.0.1 — 稳定版 | Stable Version

* **I**：修复了 Huffman 编码中的边界处理问题

* **I**: Fixed boundary handling issues in Huffman encoding

* **II**：对 EXE 文件进行了轻量化与外观优化

* **II**: Applied lightweight optimization and visual refinement to the executable

* **III**：编译优化等级由 `O2` 提升至 `O3`

* **III**: Compiler optimization level upgraded from `O2` to `O3`

---

## 关于构建 | Build Instructions

* **I**：从 Release 中下载 `.vscode` 压缩包，并放置于项目目录 `Y_Manager` 下

* **I**: Download the `.vscode` archive from the Release page and place it under the `Y_Manager` directory

* **II**：配置所需的相关依赖

* **II**: Configure the required dependencies

### 注意事项 | Notes

* 项目使用了 **OpenSSL** 库中的 `SHA256` 与 `RAND` 相关函数，
  **需自行配置 OpenSSL 依赖环境**。

* The project uses **OpenSSL** functions such as `SHA256` and `RAND`;
  **OpenSSL dependencies must be configured manually**.

* 项目使用了 **Windows 10–11 API** 以统一控制字符编码，
  **目前未向更低版本适配，也不具备跨平台能力**。

* The project utilizes **Windows 10–11 APIs** for character encoding control.
  It is **not backward-compatible** with earlier Windows versions and **does not support cross-platform execution**.

---

