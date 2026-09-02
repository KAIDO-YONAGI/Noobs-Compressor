# 纠错 + 结构合并（删 Schedule/DataBlocks/旧ThreadPool）+ BufferPool 实现 + 线程池调研改造计划文档

## 边界（已确认）
做：AES 纠错、删五个 Worker 死代码、删 Schedule//DataBlocks//旧 ThreadPool/、新建 BufferPool/ 完整实现（不接主流程）、CMake 合并替换、全量构建交付、五个问题的调研改造计划文档（体现你的"线程自己持有模块对象"思路）。
不做：ThreadPool v2 代码（你写）、主循环接线（保持串行）、IV 跨平台代码（仅文档提醒）、WSL/TSan、不动你未提交的《开发流程与设计细节.md》。

## 阶段 0：git 快照
删文件前 `git add -A && git commit` 留底（含你未提交的 .gitignore/文档改动，koharu 分支）。

## 阶段 1：AES 纠错（测试先行，行为不变）
- 新建 `tests/test_aes.cpp` + `tests/CMakeLists.txt`（MinGW、无 Qt，对应你文档 646 行"单独 cmake"意图）：round-trip（0/1/15/16/17/255/4096/1MB/8MB±1、全0/全FF/高偏斜）、IV 随机性、实例复用、截断抛错、错钥乱码。
- 对未修复代码跑基准（预期 PASS，三 bug 潜伏）→ 修三处 → 回归全绿：
  1. `mainCircle.cpp:22` providers 2 元素却 i<3 → range-for；
  2. `My_Aes.h:121` 死成员 `aesKey` → 删；
  3. `AesEncryption.h` encrypt/decrypt 的 `clear()+resize()` 无效语句 → 删，注释改"输出 = 16B IV + 密文"。

## 阶段 2：死代码删除与结构合并
- 删：heffman 五个 Worker（GetFreq/DoEncode/DoDecode/GenHeffcodeTab/LoadHeffcodeTab 的 .h+.cpp 共 10 文件，其中 GetFreq.cpp/DoEncode.cpp 是旧 ThreadPool 仅有的使用者）、`Schedule/` 整目录、`DataBlocks/` 整目录、`ThreadPool/` 旧六文件——即"ThreadPool 和 Schedule 合并"的清场：旧实现全部退场，目录留给你写 v2。
- 剥离 `Heffman.h:5`、`hefftype/Heffman_type.h:9` 对 DataBlocksManage.h 的 include（先 grep 确认无类型依赖）。
- 保留：Heffman.h/.cpp（核心算法）、HuffmanCompression.h（主流程适配器）、hefftype 其余文件。

## 阶段 3：BufferPool 完整实现（"DataBlocks 改成 BufferPool"的落地）
- 新建 `BufferPool/BufferPool.h/.cpp`：全局单池；`acquire()` 空则 cv.wait（背压语义）；`release()` 只清长度不清内容 + notify；构造预分配 8MB 块；元素为 Y_flib::DataBlock。暂不接主流程（接线方案在文档里留给你）。
- `tests/test_bufferpool.cpp`：借还守恒、空池阻塞与唤醒、reset 只清长度语义；并入 tests/CMakeLists。

## 阶段 4：CMake 合并替换 + 全量构建
- `Y_Manager_GUI/CMakeLists.txt`：去掉被删目录的 include 与源文件（57-67、72-101 行一带），加入 BufferPool；顺手修坏路径 `CompressorFileSystem/include` → `CompressorFileSystem/DataCommunication/include`。
- 走已验证的 MinGW Makefiles + 静态 Qt 6.2.4 路径全量构建（复用 Y_Manager_GUI/build；out/build 的 MSVC 缓存不自洽，弃用）→ `bin/SFC/SimpleFilesCompressorGUI.exe`。
- 跑 test_aes、test_bufferpool 全绿；主循环未动，行为与之前一致。

## 阶段 5：《线程池调研与改造计划》文档（新建 `DevelopmentMarkdowns/线程池调研与改造计划.md`，只写不实现）
按你的五个问题组织，写文档时逐文件核对行号不凭记忆：
1. **ThreadPool 与 Schedule 合并方案**：v1 问题清单（带证据行号：_Thread.cpp:10-16 永久 wait + :25 join 死锁、命名专有队列负载不均、任务异常即 terminate、无完成同步、threadNums 冗余）；v2 目标设计（共享队列+抢任务、submit→future/packaged_task、drain 析构、resize/waitIdle、Y_LightLockQueue 定稿）。
2. **DataBlocks→BufferPool 方案**：本轮已实现的部分（接口/语义/单测结论）+ 你接线时的用法（池容量 = 在途块上限 = 背压）。
3. **Huffman 改造确认**：五个 Worker 是旧池残留死代码（本轮已删）；核心算法零修改；每线程独立实例的格式依据（每 8MB 块独立树）。
4. **并发不安全模块调研（明确结论）**：逐模块——Aes（iv/buffer 成员覆写，共享实例不安全；轮密钥/S盒构造后只读）、Huffman（treeRoot/频率表/mergeTtabs 可变状态）、DataLoader/DataExporter（文件偏移状态，属"单线程持有"而非不安全）、GUI 已 QThread 隔离；**总结论：全部为"可变成员状态"类，无算法级不安全，per-thread 实例即可消除**。
5. **全局对象池方案与必要性**：不池化工具对象（微秒级构造，负收益）；池 8MB 块的必要性=背压+内存峰值控制（复用收益仅~1%）。
6. **你的总思路定稿**：WorkerEnv——线程自己持有模块对象（type_index→unique_ptr 槽表、惰性创建、线程生命周期复用），不按旧 Worker 路由思路。
7. **落地清单**：接线形态建议（专职读线程→计算池→专职写线程 vs future 收割）、IV 平台依赖（CryptGenRandom→generateRandomIV）、验收测试清单（计数守恒/异常重抛/析构不卡死/round-trip）。

## 交付物
- `SimpleFilesCompressorGUI.exe`（bin/SFC/）——你做全流程手测（四种策略 压缩→解压 一致性）。
- test_aes / test_bufferpool 两个测试 exe 及运行结果。
- 《线程池调研与改造计划》文档。