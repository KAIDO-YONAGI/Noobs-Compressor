简体中文 | [English](README.en.md)

# SFC.exe（仅支持 Windows）

---

# 项目简介

**SFC.exe（SimpleFilesCompressor）** 是一个基于 **Huffman 编码与 AES 加密**实现的文件归档工具，支持对**多个文件与目录进行压缩与解压缩**。

该项目主要用于研究**文件系统归档结构、数据压缩以及基础加密实现**。

**当前提供两个版本**：
- **命令行版本（CLI）**：`Y_Manager/` 目录，v1.x.x
- **图形界面版本（GUI）**：`SFC_GUI/` 目录，v2.x.x

---

# 功能特性

- 支持**多个文件与目录的归档压缩与解压缩**

- 使用 **Huffman 编码**实现数据压缩

- 支持 **AES 加密**与 **SHA256** 相关安全功能

- 采用 **分块压缩 / 解压机制**，降低内存占用

- 引入 **逻辑根目录（Logical Root）结构**以统一归档路径

- 实现 **路径重复跳过机制**以避免部分重复文件处理

- 支持 **Windows 文件符号链接、目录符号链接与 Junction**：归档只保存链接类型、名称和原始目标路径，不解析或递归扫描目标；外部目标与缺失目标均按正常链接保存

- **GUI 版本特性**：拖放支持、实时进度、日志输出

---

# 性能说明

由于 **Huffman 编码与 AES 加密均为纯软件实现**，程序运行速度相对较慢。

压缩算法仅依赖 Huffman，因此**压缩率有限**，最佳情况下约 **60%**。

程序采用**分块处理策略**，在路径长度正常的情况下：

- **内存峰值约为 100MB**

测试环境：

- 约 **13,000 个文件与目录**

- 总数据量约 **230GB**

---

# 安全提示

虽然程序已经实现：

- **逻辑根目录机制**
- **路径重复跳过机制**

但在极端情况下仍**可能发生文件覆盖**。

在使用软件前，请务必**备份原始文件**。

解压创建 Windows 符号链接时，系统需要启用开发者模式或授予"创建符号链接"权限；Junction 不依赖该权限。

链接目标不会随归档复制。解压到另一台机器后，目标路径不存在时会形成正常的悬空链接；绝对目标仍指向归档中记录的原位置。

---

# 项目文档

项目相关文档包括：

- `instructions.md`
  项目说明与 HuffmanZip 相关设计文档

- `策划.md`
  项目设计规划文档

- `开发日志.md`
  开发时间线与技术细节

---

# 版本历史

## v1.0.0 — Preview

存在少量已知 Bug。

主要原因是 **Huffman 树在处理单字符输入时构建不正确**。

---

## v1.0.1 — Stable

修复 Huffman 编码边界处理问题。

对 EXE 文件进行了轻量化优化与外观调整。

编译优化等级从 **O2 提升到 O3**。

---

## v1.1.1 — Well Done (CLI)

封装了文件 I/O 以及若干辅助方法（例如 `seek*` 系列函数）。

修复了文件目录分块处理问题。

内存占用稳定在 **60MB 左右**。

---

## v2.0.0 — GUI Release (2026-04-16)

**新增功能**：
- Qt 6 图形用户界面
- 左右两列布局：左列输入配置，右列进度输出
- 支持拖放文件/目录
- 实时进度显示和日志输出
- 精简部署（减少约 30MB）

---

## v2.1.0 — Strategy (2026-04-16)

**新增功能**：
- 策略模式重构：支持 4 种压缩/加密模式
  - Huffman + AES（向后兼容旧 .sy 文件）
  - Huffman Only（默认，仅压缩，无加密）
  - AES Only（仅加密，无压缩）
  - Pack Only（仅打包，无压缩无加密）
- 解压端自动检测：从文件头策略号自动选择对应模块，无需手动指定
- GUI 压缩页新增模式选择器
- GUI 解压页新增子文件夹名输入框和重置按钮
- **Qt 6.2.4 静态链接**：部署体积从 57MB 缩减至 16MB（单 exe，无 DLL 依赖）
- 背景图片优化：PNG → JPEG，嵌入资源从 11MB 减至 1.8MB
- 修复解压中文输出目录乱码问题

**架构变更**：
- 新增 `NullCompression` / `NullEncryption` 空实现
- 新增 `StrategyFactory` 策略工厂
- `CompressionLoop` / `DecompressionLoop` / `BinaryStandardLoader` 全面改为接口引用（`ICompression&` / `IEncryption&`）
- 文件头 `strategy` 字段正式启用（原已预留但始终为 0）
- `CMakeLists.txt` 新增 `USE_STATIC_QT` 开关，支持静态/动态链接切换
- LGPL v3 合规声明

---

# 构建指南

## CLI 版本构建

如需自行编译命令行版本：

### 1 下载构建配置

从 Release 页面下载 `.vscode` 压缩包。

---

### 2 放入项目目录

将解压后的 `.vscode` 文件夹放入：

```
Y_Manager/
```

---

### 3 编译程序

编译 `main.cpp` 文件即可生成可执行程序。

---

## GUI 版本构建

GUI 版本位于 `SFC_GUI/` 目录，需要 Qt 6.2.4 LTS 环境。

### 编译要求

- **Qt 6.2.4 LTS**（使用自带的 MinGW 11.2.0）
- **CMake 3.20+**
- **静态链接模式**：需要预编译的 Qt 6.2.4 静态库（`D:/qt/6.2.4-static-mingw/`）

### 构建步骤（静态链接）

使用 `build_static.bat` 脚本构建（推荐）：

```bash
# 在 cmd.exe 中运行
cd SFC_GUI
build_static.bat
```

或手动构建：

```bash
cd SFC_GUI
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DUSE_STATIC_QT=ON
cmake --build build --parallel
```

构建完成后，单 exe 位于 `bin/SFC/` 目录，无需任何 DLL。

### 构建步骤（动态链接）

```bash
cd SFC_GUI
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DUSE_STATIC_QT=OFF
cmake --build build --config Release -j 8
```

构建完成后，可执行文件位于 `bin/SFC/` 目录。

---

# 系统要求

## CLI 版本

- **操作系统**：Windows 10 及以上
- **架构**：x64

## GUI 版本

- **操作系统**：Windows 10 1607+（静态链接版本）/ Windows 10 1809+（动态链接版本）
- **架构**：x64
- **部署体积**：静态链接 16MB（单 exe）/ 动态链接 57MB（exe + DLL + 插件）

---

### 编译要求

- 使用 **C++20 标准**

- 编译器选项：

```
-std=c++20
```

- 推荐优化等级：

```
-O3
```

- **不要启用 LTO（链接时优化）**

- **GUI 版本必须使用 Qt 自带的 MinGW**

---

# 依赖

## CLI 版本

- **OpenSSL**
  - `SHA256`
  - `RAND`

- **Windows 10–11 API**
  用于字符编码控制以及随机数生成

## GUI 版本

- **Qt 6.2.4 LTS** (Core, Widgets)
- **MinGW 11.2.0**（Qt 自带）
- **Windows 10–11 API**
- **静态链接模式额外依赖**：
  - Qt 6.2.4 静态库（`D:/qt/6.2.4-static-mingw/`）
  - Windows 系统库：dwmapi, uxtheme, imm32, oleaut32, version, setupapi, fontsub

---

# 免责声明

本项目仅用于 **教学与学习目的**。

使用前请务必 **备份原始文件**。
