# BuildTest/_bench —— 开发期临时产物目录

本目录放**只在开发/测量期间存在**的东西，随时可以整目录 `Remove-Item -Recurse` 掉，
不影响构建、测试与仓库状态。

## 放什么

| 类别 | 例子 |
|---|---|
| 微基准 harness | 分阶段吞吐测量、消融实验、best-of-N 对比 |
| 一次性探针 | CPU 指令集探测、加密库各模式可用性/吞吐对比 |
| 黄金快照 | 用于"逐字节/逐位一致"回归的基线输出 |
| 测量日志 | 构建日志、A/B 结果、性能对照表 |
| 发版草稿 | Release note 文本、发布说明 |

## 不放什么

- **不放源码**：任何需要长期维护的代码（包括测试）都在
  `BuildTest/tests/`，由 `BuildTest/tests/CMakeLists.txt` 收录。
- **不放夹具**：端到端测试的样本集由 `ctest` 在运行时生成到
  `BuildTest/tests/build/ctest_fixture/`（见 `fixture_sample`），不入库。
- **不放需要复现的结果**：结论本身要落进 `DevFiles/`（开发日志、
  设计文档、方法论），本目录只留过程产物。

## 为什么在这里

- 在仓库内，方便就近访问模块源码、复用 `BuildTest/` 已有的编译器与
  CMake 配置，不会像仓库外目录那样被误删或丢失。
- 由 `.gitignore` 的 `BuildTest/_bench/*` 规则排除，且只有本 README 入库，
  所以不会污染 `git status`，也不会被误提交。

## 编译示例

本目录不在任何 CMake 工程内，单独编译即可（工具链与主构建一致）：

```powershell
$g = 'D:\qt\Tools\mingw1120_64\bin\g++.exe'
$r = 'D:\My_Docs\Programmes\Simple Files Compressor'
& $g -std=c++20 -O3 -DNDEBUG -flto `
   -I "$r/CompressionModules/Huffman/Core/include" `
   -I "$r/CompressionModules/Huffman/Hufftype" `
   -I "$r/BufferPool" `
   -I "$r/CompressorFileSystem/Commons/include" `
   "$r/BuildTest/_bench/codec_bench.cpp" `
   "$r/CompressionModules/Huffman/Core/src/Huffman.cpp" `
   "$r/CompressionModules/Huffman/Hufftype/HuffmanType.cpp" `
   -o "$r/BuildTest/_bench/codec_bench.exe"
```

口径提醒：与正式测量对齐时用 `-O3 -DNDEBUG`（加 `-flto` 是与发布构建同口径），
单线程、`best-of-N`，并逐字节校验输出一致。
