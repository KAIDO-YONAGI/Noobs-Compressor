# 项目验证指南

文档 ID：`WF-PROJECT-VALIDATION`  
状态：`Reference`  
维护计数：`0/5`  
最后更新：`2026-10-26`

## 范围

本文件只记录 **Simple Files Compressor 仓库中确实存在** 的验证命令与状态变更。
状态为 `Reference`：命令与 `README.md`、`BuildTest/` 下的 CMake 脚本一致，尚未在本仓库
执行记录中登记通过结果。执行并确认后改为 `Active`。

不得把其它项目的构建、发布或运行流程套用到这里。

## 触发条件

出现以下改动时，应按对应层级验证：

| 改动范围 | 最少验证 |
|---|---|
| 仅文档（`Y_MultipleAgentWorkflow/`、`README.md`） | 不需要构建 |
| `CompressionModules/`、`EncryptionModules/` | 对应单元测试 + 端到端 roundtrip |
| `CompressorFileSystem/`、`BufferPool/`、`ThreadPool/` | 全部单元测试 + 端到端 roundtrip |
| `Y_Manager/`、`SFC_GUI/` | 完整构建 + 全部测试 |
| 归档格式、常量、`FileLibrary.h` | 完整构建 + 全部测试 + 更新格式规范 |

## 构建命令

### 静态链接（默认，产出单 exe）

```powershell
cmake -S BuildTest -B BuildTest/build -DCMAKE_BUILD_TYPE=Release -DUSE_STATIC_QT=ON
cmake --build BuildTest/build --parallel
```

`USE_STATIC_QT` 在 `BuildTest/CMakeLists.txt` 中默认为 `ON`。静态模式要求预编译的
Qt 6.2.4 静态库。

### 动态链接

```powershell
cmake -S BuildTest -B BuildTest/build -DCMAKE_BUILD_TYPE=Release -DUSE_STATIC_QT=OFF
cmake --build BuildTest/build --parallel
```

## 测试命令

```powershell
cmake -S BuildTest/tests -B BuildTest/tests/build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build BuildTest/tests/build --parallel
ctest --test-dir BuildTest/tests/build --output-on-failure
```

已注册的测试名（来自 `BuildTest/tests/CMakeLists.txt`）：

| 测试名 | 可执行文件 | 覆盖点 |
|---|---|---|
| `unit_aes` | `test_aes` | AES 加解密与密钥派生 |
| `unit_bufferpool` | `test_bufferpool` | 缓冲池借出/归还与背压 |
| `unit_huffman` | `test_huffman` | 哈夫曼编解码往返 |
| `unit_flagtype` | `test_flagtype` | 各类标准的序列化与解析 |
| `unit_safequeue` | `test_safequeue` | 队列阻塞、唤醒与关闭 |
| `unit_writesorter` | `test_writesorter` | 序号重排 |
| `fixture_sample` | `tool_archivebaseline` | 生成长路径/中文目录样本 |
| `e2e_roundtrip_<模式>` | `tool_archivebaseline` | 各压缩模式的端到端往返 |

## 成功标准

- 配置与构建退出码为 0，无未处理警告升级为错误。
- `ctest` 全部通过；失败时用 `--output-on-failure` 查看用例输出。
- 端到端 roundtrip 比对通过，即解压结果与 baseline 目录逐文件一致。

## 进程与状态

- 构建与测试都只写 `BuildTest/build/`、`BuildTest/tests/build/` 与 fixture 目录，
  不修改源码树以外的东西。
- `BuildTest/tests/build/` 下的 fixture 残留会影响后续比对，必要时整目录删除后重跑。
- 构建或测试期间不要并发修改源码，对应全局屏障 `pipeline:BuildPublishRun`。

## 环境

- Qt 6.2.4（Core、Widgets），使用其自带 MinGW 11.2.0。
- CMake 3.20 及以上。
- 静态链接模式需要预编译的 Qt 6.2.4 静态库。
- 详细依赖与第三方许可见仓库根 `README.md` 与 `THIRD_PARTY_NOTICES.md`。
