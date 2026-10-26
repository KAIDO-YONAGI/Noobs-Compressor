# SFC — Simple Files Compressor

简体中文 | [English](README.en.md)

SFC 是 Windows 平台的图形化归档工具。它把多个文件与目录收进一个 `.sy` 归档，按 8 MiB 分块
做 Huffman 压缩，按需对数据块与目录块做 AES-128-CTR 加密，解压时按归档内记录的目录结构
还原，符号链接与 Junction 一并重建。

版本 2.2.1，归档格式 v3。

## 1. 能力

| 能力 | 说明 |
|---|---|
| 多输入 | 一次任务加入多个文件与多个目录，整目录可直接拖入 |
| 四种处理模式 | Huffman、Huffman + AES、AES、Pack 四种组合 |
| 三段流水线 | 一个读线程、N 个计算工人、一个写线程，N 默认取 CPU 逻辑核数 |
| 内存与输入规模无关 | 峰值随工人数与块大小变化 |
| 单一逻辑根 | 全部条目挂在同一个根目录下，根名取归档文件名，条目名记相对路径 |
| 输入去重 | 列表中已有的完整路径不再重复加入 |
| Windows 链接 | 符号链接与 Junction 记录类型、名称与目标，扫描阶段不跟随 |
| 界面 | 拖放、实时进度、逐文件日志、中文译文 |

## 2. 处理模式

模式在压缩页右栏选择，默认 Huffman Only。模式号写入文件头 `strategy` 字节，解压端据此选择
读取流程。

| 模式 | `strategy` | 压缩 | 加密 |
|---|---|---|---|
| Huffman + AES | 0 | Huffman | AES-128-CTR |
| Huffman Only | 1 | Huffman | 无 |
| AES Only | 2 | 无 | AES-128-CTR |
| Pack Only | 3 | 无 | 无 |

Huffman + AES 与 AES Only 在写入前要求口令。加密覆盖数据块与目录块；Pack Only 与
Huffman Only 的目录区保持明文。

## 3. 归档结构

```text
[ 文件头 19 字节 ][ 目录区 ][ 目录尾魔数 ][ 数据区 ]
```

- 文件头：起始魔数、`strategy`、`version`、文件头长度、目录区偏移、第二个魔数。
- 目录区：由目录块串成，块体累计到 16 KiB 时换下一块，每块前有 25 字节分割标准；块内按条目
  类型记录目录、文件、逻辑根、符号链接与 Junction。
- 目录尾魔数：4 字节。目录区偏移把它计入，数据区从这一偏移开始。
- 数据区：每个源文件按 8 MiB 切块，每块写一个元数据块再写一个数据块。Huffman 的元数据是
  258 字节码长表，数据块体首字节记录末尾填充位数，其后是码流；AES 模式下每块块体前 16 字节
  是该块的随机 IV。

读取端接受 `version` 为 3 的归档，并逐处校验三处魔数。

逐字段规范、偏移算法与最小编码样例见[《归档格式》](Y_MultipleAgentWorkflow/ArchiveFormat/归档格式.md)。

## 4. 使用

### 压缩

1. 用左栏按钮或拖放把文件与目录加入列表。
2. 确认输出目录与输出文件名。加入第一个条目后两者自动填好，可以改。
3. 在右栏选择模式。加密模式要填写口令。
4. 点「Start Compression」。归档写入输出目录，扩展名为 `.sy`。

### 解压

1. 选择一个 `.sy` 归档，也可以直接把文件拖进窗口。
2. 选择输出目录。「Subfolder Name」填一个子目录名，最终根目录为输出目录下的该子目录。
3. 加密归档要填写压缩时用的同一口令，口令为空时任务不启动。
4. 点「Start Decompression」。归档内的逻辑根目录建在上一步的根目录下。

### 运行期

右栏显示整体进度、当前文件进度与逐行日志，日志保留最近 500 行。任务进行中关闭窗口即请求
停止，主循环在阶段间隙退出，已写出的文件保留在目标目录。

### 环境变量

| 变量 | 作用 |
|---|---|
| `SFC_WORKERS` | 计算工人数，取正整数。未设置时取硬件并行度，取不到时用 4；压缩与解压同口径 |
| `SFC_GUI_LANGUAGE` | 界面语言，形如 `zh_CN`；`system` 表示跟随系统 |

界面串以英文为原文。首选语言为中文时加载中文译文，其余情况显示英文原文。

## 5. 性能

端到端数字来自 13.47 GiB / 45,390 个文件的真实工程树，模式 Huffman + AES，工人数取默认值
16，走完整读、算、写流程并含磁盘 I/O：

| 方向 | 吞吐 |
|---|---|
| 压缩 | 254.3 MiB/s |
| 解压 | 325.1 MiB/s |

内核数字为单线程、每 8 MiB 一块、重复测量取最快一次：

| 阶段 | 吞吐 |
|---|---|
| Huffman 编码 | 471.1 MiB/s |
| Huffman 解码 | 236.8 MiB/s |
| AES 加密 | 1261.5 MiB/s |
| AES 解密 | 1279.6 MiB/s |

两组口径不同，不能互相换算。硬件为 AMD Ryzen 7 8845H，8 物理核 16 逻辑处理器；构建口径
`-O3 -DNDEBUG` 并开 LTO。端到端吞吐止步于单读线程与单写线程，多核算力在计算段已有余量。

内存峰值 ≈ `2 × (N + 2)` 个池块加 `2 × N` 个工人私有块，每块 8 MiB。16 工人全量压缩实测
约 500 MB，峰值与输入总规模无关。

压缩率取决于输入内容：高可压缩样本 2,048 MiB 归档为 896 MiB，近不可压缩的真实工程树保持
在原规模。

测量条件与瓶颈分析见[《工程实践》](Y_MultipleAgentWorkflow/Engineering/工程实践.md)。

## 6. 构建

### 依赖

- Qt 6.2.4 LTS，组件 Core 与 Widgets
- Qt 自带的 MinGW 11.2.0
- CMake 3.20 及以上
- 静态构建另需预编译的 Qt 6.2.4 静态库

配置前把 Qt 自带 MinGW 11.2.0 的 `bin` 目录排到 `PATH` 首位，并核对
`BuildTest/CMakeLists.txt` 顶部的 Qt 安装路径。

### 静态链接，默认

```powershell
cmake -S BuildTest -B BuildTest/build -DCMAKE_BUILD_TYPE=Release -DUSE_STATIC_QT=ON
cmake --build BuildTest/build --parallel
```

产物在 `BuildTest/bin/SFC/`，单个可执行文件，无随附 DLL。

### 动态链接

```powershell
cmake -S BuildTest -B BuildTest/build -DCMAKE_BUILD_TYPE=Release -DUSE_STATIC_QT=OFF
cmake --build BuildTest/build --parallel
```

Post-Build 阶段调用 `windeployqt` 部署 Qt DLL 与 `platforms` 插件，复制 MinGW 运行时 DLL，
并删去网络、TLS、样式等运行中用不到的文件。

### 测试

```powershell
cmake -S BuildTest/tests -B BuildTest/tests/build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build BuildTest/tests/build --parallel
ctest --test-dir BuildTest/tests/build --output-on-failure
```

测试工程与 GUI 工程各自配置、各自构建，共用 `BuildTest/sfc_core.cmake` 里的核心源清单。
套件含六个单元测试与三个端到端往返用例，样本集由 `ctest` 运行时生成。

### 编译口径

C++20；Release 加 `-O3 -DNDEBUG`；LTO 打开，归档器由 CMake 换成工具链的 `gcc-ar` 与
`gcc-ranlib`。

## 7. 运行约束与风险

| 项 | 事实 |
|---|---|
| 口令派生 | 口令经 SHA-256 压成 32 字节摘要，取前 16 字节作 AES-128 密钥，无盐、单次迭代 |
| 完整性 | 归档没有认证标签。位翻转或被改写后解压照常完成，产出与原文不同的数据 |
| 明文范围 | 文件头、块数、每块长度与归档总长始终明文，可据此推断原始规模；AES 模式下 IV 亦明文前置 |
| 口令强度 | 实际保护强度等于口令强度，离线字典攻击成本低 |
| 归档来源 | 解压把归档内路径直接与输出目录拼接，`..` 与绝对路径都生效。只解压自己生成的归档 |
| 同名文件 | 目标位置已有同名文件时，数据追加到该文件末尾。解压前确认目标目录为空 |
| 符号链接 | 创建需要开发者模式或「创建符号链接」权限 |
| Junction | 用重解析点写入，不需要额外权限；相对目标按链接父目录换算为绝对路径，UNC 目标被拒绝 |
| 链接目标 | 按归档记录原样恢复，不随归档复制。跨机器解压后目标缺失时得到正常的悬空链接 |
| 提交 | 解压直接写最终路径，任务中止后目标目录保留已写出的部分文件 |

## 8. 系统要求

- Windows 10 x64 及以上，x64 架构
- 静态构建无需第三方运行时；动态构建需要 Qt 运行库

## 9. 许可

- 本项目自身代码采用 GPL-3.0，完整原文见 [LICENSE](LICENSE)。
- 第三方组件为 Qt 6.2.4，LGPL v3，静态链接。合规信息与资源授权登记见
  [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## 10. 文档

项目全部权威文档的统一入口是 [`Y_MultipleAgentWorkflow/Router.md`](Y_MultipleAgentWorkflow/Router.md)，
按任务路由到架构、归档格式、流水线、编解码内核、界面、工程实践各自的文档，并声明权威顺序
与维护规则。
