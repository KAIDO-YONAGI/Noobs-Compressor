English | [简体中文](README.md)

# SFC.exe (Windows Only)

---

# Project Overview

SFC.exe (SimpleFilesCompressor) is a file archiving tool based on Huffman coding and AES encryption, supporting compression and decompression of multiple files and directories.

The project is mainly intended for studying file archiving structures, data compression, and basic encryption implementations.

The code is organised by responsibility:

- `SFC_GUI/`: Qt graphical interface and program entry point
- `Y_Manager/`: compression and decompression pipelines
- `CompressorFileSystem/`: archive format, directory standard parsing and I/O
- `CompressionModules/`: Huffman codec
- `EncryptionModules/`: AES encryption
- `ThreadPool/`, `BufferPool/`: concurrency components and buffer pool
- `BuildTest/`: build entry point and unit tests

---

# Features

- Archiving, compression, and decompression of multiple files and directories
- Huffman coding for data compression
- AES encryption and SHA-256 key derivation
- Block-based compression and decompression mechanism to reduce memory usage
- A logical root directory structure to manage archive paths
- A duplicate-path skipping mechanism to avoid redundant processing
- Windows file symbolic links, directory symbolic links, and Junctions: only the link type, name and original target path are stored, without resolving or scanning the target; external and missing targets are preserved normally
- GUI: drag-and-drop support, real-time progress, log output, cancellation and localisation

---

# Performance Notes

Both compression and decompression are 3-stage pipelines: a dedicated reader thread, N compute workers, and a dedicated writer thread, where N is the number of logical CPU cores. The worker count can be overridden with the `SFC_WORKERS` environment variable. AES uses the standard AES-128-CTR implementation from Windows CNG and therefore runs on the AES-NI hardware instructions; Huffman coding/decoding remains pure software.

Measured throughput on an input of 13.47 GiB / 45,390 files:

| Direction | Throughput (MiB/s) |
|---|---|
| Compress | 254.3 |
| Decompress | 325.1 |

Single-threaded codec throughput, 8 MB per block:

| Stage | Throughput (MiB/s) |
|---|---|
| Huffman encode | 471.1 |
| Huffman decode | 236.8 |
| AES encrypt | 1261.5 |
| AES decrypt | 1279.6 |

Per-block metadata is 258 bytes.

Because compression relies solely on Huffman coding, the compression ratio is limited, with a best-case ratio of about 60%.

Peak memory is independent of total input size and has two components: the buffer pool of `2 x (workers + 2)` 8 MB blocks, about 288 MB with 16 workers, plus the per-worker private 8 MB scratch buffer, about 128 MB with 16 workers. On top of that sits the process image itself, about 90 MB when idle with static Qt. On a 16-worker / 8 MB-block machine the measured peak during a full run is about 500 MB.

Multi-core utilisation, bottleneck analysis and the measured per-version evolution are in [《性能报告》](DevFiles/性能报告.md); the general criteria and methodology behind these speedups (three criteria, five audits, a decision tree, cross-language mapping, an anti-pattern checklist and how to identify which kind of bottleneck you have) are in [《性能优化通用方法论》](DevFiles/性能优化通用方法论.md). Both documents are Chinese only for now.

---

# Safety Notice

Although the program includes a logical root directory mechanism and a duplicate-path skipping mechanism, file overwriting may still occur in extreme situations.

Please always back up your original files before using the software.

Restoring Windows symbolic links requires Developer Mode or the "Create symbolic links" privilege; Junction restoration does not require it.

Link targets are not copied into the archive. On another machine, a missing target becomes a normal dangling link, while an absolute target continues to point to the recorded location.

---

# Project Documentation

- `DevFiles/开发流程与设计细节.md`: program structure, archive format, current implementation and thread model
- `DevFiles/性能报告.md`: multi-core utilisation, bottleneck analysis and measured per-version evolution
- `DevFiles/性能优化通用方法论.md`: general methodology for memory access, billing granularity and parallelism
- `DevFiles/开发日志.md`: development timeline and technical details
- `DevFiles/Others/策划.md`: project design plan
- `DevFiles/Others/Instructions.md`: project notes and HuffmanZip design documents
- `DevFiles/Others/Qt技术要点与策略工厂详解.md`: Qt and strategy factory notes

---

# Build Instructions

## Requirements

- Qt 6.2.4 LTS, using its bundled MinGW 11.2.0
- CMake 3.20+
- Static builds require a pre-built Qt 6.2.4 static library (`D:/qt/6.2.4-static-mingw/`)

## Build Steps (Static)

```bash
cmake -S BuildTest -B BuildTest/build -DCMAKE_BUILD_TYPE=Release -DUSE_STATIC_QT=ON
cmake --build BuildTest/build --parallel
```

The result is a single exe under `BuildTest/bin/SFC/`, with no DLL dependencies.

## Build Steps (Dynamic)

```bash
cmake -S BuildTest -B BuildTest/build -DCMAKE_BUILD_TYPE=Release -DUSE_STATIC_QT=OFF
cmake --build BuildTest/build --parallel
```

## Unit Tests and End-to-End Checks

```bash
cmake -S BuildTest/tests -B BuildTest/tests/build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build BuildTest/tests/build --parallel
ctest --test-dir BuildTest/tests/build --output-on-failure
```

---

# System Requirements

- OS: Windows 10 1607 or later (static build) / Windows 10 1809 or later (dynamic build)
- Architecture: x64
- Deployment size: 16 MB static (single exe) / 57 MB dynamic (exe + DLLs + plugins)

---

# Compilation Requirements

- C++20: `-std=c++20`
- Recommended optimisation level: `-O3`
- LTO (link-time optimisation) is enabled; the archiver must be `gcc-ar` / `gcc-ranlib`
- Qt's bundled MinGW is required

---

# Dependencies

- Qt 6.2.4 LTS (Core, Widgets)
- MinGW 11.2.0 (bundled with Qt)
- Windows 10-11 API
- Additional dependencies for static builds:
  - Qt 6.2.4 static library (`D:/qt/6.2.4-static-mingw/`)
  - Windows system libraries: bcrypt, dwmapi, uxtheme, imm32, oleaut32, version, setupapi, fontsub

---

# Licensing

- The project's own code: GPL-3.0 (full text in [LICENSE](LICENSE)). Free to use, modify and redistribute; redistributions must be released under the same licence and may not be closed-source.
- Third-party components: Qt 6.2.4 (LGPL v3, statically linked). Licence terms and compliance material, including the object files needed for relinking, source locations and build parameters, are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
- The icons, background images, diagrams and translations bundled with the project are also registered in that file; GPL-3.0 covers only the parts whose copyright the author holds.

---

# Disclaimer

This project is for teaching and learning purposes only.

Always back up your original files before use.
