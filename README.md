简体中文 | [English](README.en.md)

# SFC.exe（仅支持 Windows）

---

# 项目简介

SFC.exe（SimpleFilesCompressor）是一个基于 Huffman 编码与 AES 加密实现的文件归档工具，支持对多个文件与目录进行压缩与解压缩。

该项目主要用于研究文件系统归档结构、数据压缩以及基础加密实现。

代码按职责分为以下几部分：

- `SFC_GUI/`：Qt 图形界面，程序入口
- `Y_Manager/`：压缩与解压流水线
- `CompressorFileSystem/`：归档格式、目录标准解析与读写
- `CompressionModules/`：Huffman 编解码
- `EncryptionModules/`：AES 加密
- `ThreadPool/`、`BufferPool/`：并发组件与缓冲池
- `BuildTest/`：构建入口与单元测试

---

# 功能特性

- 支持多个文件与目录的归档压缩与解压缩
- 使用 Huffman 编码实现数据压缩
- 支持 AES 加密与 SHA-256 密钥派生
- 采用分块压缩 / 解压机制，降低内存占用
- 引入逻辑根目录（Logical Root）结构以统一归档路径
- 实现路径重复跳过机制以避免部分重复文件处理
- 支持 Windows 文件符号链接、目录符号链接与 Junction：归档只保存链接类型、名称和原始目标路径，不解析或递归扫描目标；外部目标与缺失目标均按正常链接保存
- GUI：拖放支持、实时进度、日志输出、取消与界面本地化

---

# 性能说明

压缩与解压都是三段流水线：专职读线程 + N 个计算工人 + 专职写线程，N 取 CPU 逻辑核数，工人数可用环境变量 `SFC_WORKERS` 覆盖。AES 使用 Windows CNG 的标准 AES-128-CTR，自动走 AES-NI 硬件指令；Huffman 编解码为纯软件实现。

当前实测吞吐，输入为 13.47 GiB / 45,390 个文件：

| 方向 | 吞吐 (MiB/s) |
|---|---|
| 压缩 | 254.3 |
| 解压 | 325.1 |

内核单线程吞吐，每 8 MB 一块：

| 阶段 | 吞吐 (MiB/s) |
|---|---|
| Huffman 编码 | 471.1 |
| Huffman 解码 | 236.8 |
| AES 加密 | 1261.5 |
| AES 解密 | 1279.6 |

每个数据块的说明信息为 258 字节。

压缩算法仅依赖 Huffman，压缩率有限，最佳情况下约 60%。

内存峰值与输入总规模无关，由两部分构成：缓冲池 `2×(工人数+2)` 个 8MB 块，16 工人约 288 MB；每个工人私有的 8MB 中间缓冲，16 工人约 128 MB。另加进程映像本身，静态 Qt 空闲时约 90 MB。16 工人 / 8MB 块的机器上，全量压缩实测峰值约 500 MB。

多核利用、瓶颈分析与版本演进实测见 [《性能报告》](DevFiles/性能报告.md)；各项优化背后的通用判据与方法论见 [《性能优化通用方法论》](DevFiles/性能优化通用方法论.md)。

---

# 安全提示

虽然程序已经实现逻辑根目录机制与路径重复跳过机制，但在极端情况下仍可能发生文件覆盖。

在使用软件前，请务必备份原始文件。

解压创建 Windows 符号链接时，系统需要启用开发者模式或授予“创建符号链接”权限；Junction 不依赖该权限。

链接目标不会随归档复制。解压到另一台机器后，目标路径不存在时会形成正常的悬空链接；绝对目标仍指向归档中记录的原位置。

---

# 项目文档

- `DevFiles/开发流程与设计细节.md`：程序结构、归档格式、当前实现与线程模型
- `DevFiles/性能报告.md`：多核利用、瓶颈分析与版本演进实测
- `DevFiles/性能优化通用方法论.md`：访存、计费粒度与并行度的通用方法论
- `DevFiles/开发日志.md`：开发时间线与技术细节
- `DevFiles/Others/策划.md`：项目设计规划
- `DevFiles/Others/Instructions.md`：项目说明与 HuffmanZip 相关设计文档
- `DevFiles/Others/Qt技术要点与策略工厂详解.md`：Qt 与策略工厂要点

---

# 构建指南

## 编译要求

- Qt 6.2.4 LTS，使用自带的 MinGW 11.2.0
- CMake 3.20+
- 静态链接模式需要预编译的 Qt 6.2.4 静态库（`D:/qt/6.2.4-static-mingw/`）

## 构建步骤（静态链接）

```bash
cmake -S BuildTest -B BuildTest/build -DCMAKE_BUILD_TYPE=Release -DUSE_STATIC_QT=ON
cmake --build BuildTest/build --parallel
```

构建完成后，单 exe 位于 `BuildTest/bin/SFC/` 目录，无需任何 DLL。

## 构建步骤（动态链接）

```bash
cmake -S BuildTest -B BuildTest/build -DCMAKE_BUILD_TYPE=Release -DUSE_STATIC_QT=OFF
cmake --build BuildTest/build --parallel
```

## 单元测试与端到端校验

```bash
cmake -S BuildTest/tests -B BuildTest/tests/build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build BuildTest/tests/build --parallel
ctest --test-dir BuildTest/tests/build --output-on-failure
```

---

# 系统要求

- 操作系统：Windows 10 1607 及以上（静态链接版本）/ Windows 10 1809 及以上（动态链接版本）
- 架构：x64
- 部署体积：静态链接 16MB（单 exe）/ 动态链接 57MB（exe + DLL + 插件）

---

# 编译要求

- 使用 C++20 标准：`-std=c++20`
- 推荐优化等级：`-O3`
- 启用 LTO（链接时优化），归档器需使用 `gcc-ar` / `gcc-ranlib`
- 必须使用 Qt 自带的 MinGW

---

# 依赖

- Qt 6.2.4 LTS（Core, Widgets）
- MinGW 11.2.0（Qt 自带）
- Windows 10–11 API
- 静态链接模式的额外依赖：
  - Qt 6.2.4 静态库（`D:/qt/6.2.4-static-mingw/`）
  - Windows 系统库：bcrypt, dwmapi, uxtheme, imm32, oleaut32, version, setupapi, fontsub

---

# 授权 / Licensing

- 本项目自身代码：GPL-3.0（完整原文见 [LICENSE](LICENSE)）。可自由使用、修改、再分发；再分发时必须以同一许可证开放源码，不得闭源发布。
- 第三方组件：Qt 6.2.4（LGPL v3，静态链接）。许可条款与合规材料（重新链接所需的对象文件、源码地址、构建参数）见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
- 本项目自带的图标、背景图、流程图与翻译等资源同样登记在该文件中；GPL-3.0 只覆盖作者拥有版权的部分。

---

# 免责声明

本项目仅用于教学与学习目的。

使用前请务必备份原始文件。
