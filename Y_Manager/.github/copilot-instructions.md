# Copilot 使用说明（项目特定）

下面的说明帮助 AI 代码代理快速在此仓库中高效工作。只记录可从代码/配置中发现的、对编码和修改有直接影响的信息。

## 项目概览（大局）
- **单一可执行测试驱动仓库**：当前工作入口是 [main.cpp](main.cpp)。该文件用作手动/调试的运行器，演示两条主要功能路线：HeaderWriter（注释掉）和 HeaderLoader/DataLoader/DataExporter（当前启用）。
- **数据流组件位置**：源码通过相对 include 引用 DataStream/DataCommunication 下的组件（示例：`../DataStream/DataCommunication/include/HeaderLoader.h`）。这些模块负责读取压缩文件、排队文件元数据并逐段导出数据。
- **运行时假设**：项目在 Windows 环境下使用 MSYS2 的 `g++`（ucrt64），代码中大量使用绝对 Windows 路径字符串和 UTF-8/非 ASCII 文件名（例如 “挚爱的时光.bin”）。

## 关键组件与模式（可从代码中直接观察）
- `HeaderWriter` / `HeaderLoader`：负责目录扫描与元数据写入/加载；`HeaderLoader` 提供 `fileQueue`（队列），主循环从该队列中取文件并交由 `DataLoader` 处理。
- `DataLoader`：具备 `dataLoader()`, `isDone()`, `getBlock()`, `reset(path)` 等生命周期方法；典型使用为反复调用 `dataLoader()`，在完成后取 `getBlock()` 写入 `DataExporter`。
- `DataExporter`：提供 `exportDataToFile_Encryption()` 和 `thisFileIsDone(offset)` 等接口，用于接收块并标记文件完成。
- 约定：主循环先 `dataLoader()`，当 `isDone()` 为真时，调用 `thisFileIsDone()` 并从 `headerLoader.fileQueue` pop 出当前文件，然后用 `reset()` 更新下个文件路径。

## 构建 / 调试（可直接运行的步骤）
- 现有 VS Code 任务（可直接使用）：

  - 命令（在任务中）：

    C:\\msys64\\ucrt64\\bin\\g++.exe -g ${file} -o ${fileDirname}\\${fileBasenameNoExtension}.exe

  - 工作目录：`C:\\msys64\\ucrt64\\bin`

- 使用建议：在修改多文件或引入新的 include 目录时，更新你的编译命令以包含 `-I`（头文件）和 `-L` / `-l`（库）选项，或改为使用 CMake/Makefile 来管理复杂依赖。

## 项目约定与注意事项
- 路径：代码里混用相对 include（`../...`）和 Windows 绝对路径字符串。修改路径时请保持 Windows 路径格式（反斜杠）并注意转义。测试时可用 MSYS2 的 g++，也可以用 MinGW/wsl 编译，但注意编码与路径差异。
- 字符编码：仓库示例使用非 ASCII 的输出文件名；保持源文件 UTF-8 编码以避免运行时/链接时误读文件名。
- 测试方式：`main.cpp` 是手动测试台，常通过更换注释区切换测试场景（Writer vs Loader）。若添加自动测试，优先创建一个小的 test/ 目录并编写可复现的路径（避免依赖本机绝对路径）。

## 改动建议（当你要实现新功能或修复时）
- 新增模块要在 `include` 路径下放置对应头并在 `main.cpp` 的测试部分清晰展示用法。
- 若引入压缩/加密算法，保持 `DataLoader` -> `DataExporter` 的“块（block）传递”模式，不要让导出直接操作队列索引；保持单向数据流，便于重用。

## 快速示例（从仓库摘取）
- 主循环（简化）示例来自 `main.cpp`：

```
while (!headerLoader.fileQueue.empty()) {
  dataLoader.dataLoader();
  if (dataLoader.isDone()) {
    dataExporter.thisFileIsDone(offset);
    headerLoader.fileQueue.pop();
    dataLoader.reset(nextPath);
  } else {
    dataExporter.exportDataToFile_Encryption(dataLoader.getBlock());
  }
}
```

## 当不确定时应该做什么
- 先运行 `main.cpp` 的当前测试场景以观察运行时行为与路径依赖，再修改。
- 避免直接改动硬编码的绝对路径——若必须改动，请同时更新 `main.cpp` 的示例路径和 README（若存在）。

---
如果这些信息有不完整或错误之处，请指出具体关心的区域（构建、编码风格、组件 API 或测试流程），我会按需迭代此文件。
