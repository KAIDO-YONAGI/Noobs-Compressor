English | [简体中文](README.md)

# SFC.exe (Windows Only)

---

# Project Overview

**SFC.exe (SimpleFilesCompressor)** is a file archiving tool based on **Huffman coding and AES encryption**, supporting **compression and decompression of multiple files and directories**.

The project is mainly intended for studying **file archiving structures, data compression, and basic encryption implementations**.

**Two versions are available**:
- **Command-line version (CLI)**: `Y_Manager/` directory, v1.x.x
- **GUI version**: `SFC_GUI/` directory, v2.x.x

---

# Features

- Supports **archiving, compression, and decompression of multiple files and directories**

- Uses **Huffman coding** for data compression

- Supports **AES encryption** and **SHA256** related security functions

- Uses a **block-based compression and decompression mechanism** to reduce memory usage

- Introduces a **logical root directory structure** to manage archive paths

- Implements a **duplicate-path skipping mechanism** to avoid redundant processing

- Supports **Windows file symbolic links, directory symbolic links, and Junctions** by storing their type, name, and original target path without resolving or scanning the target; external and missing targets are preserved normally

- **GUI features**: drag-and-drop support, real-time progress, log output

---

# Performance Notes

The compression core is a 3-stage pipeline (dedicated reader thread + N compute workers + dedicated writer thread, N = logical CPU cores). Huffman coding and AES encryption are pure software implementations (no AES-NI).

Measured compression throughput in v2.2.0 (HuffmanAES, 16 logical cores / NVMe, same machine and identical input vs the old version):

| Data profile | Old throughput | v2.2.0 throughput | Speedup |
|---|---|---|---|
| Mostly large files (avg 1.1 MB/file) | 19.1 MiB/s | 131.9 MiB/s | **6.9x** |
| Small/medium files + very long paths (avg 75 KB/file, up to 267 chars) | 14.7 MiB/s | 52.6 MiB/s | **3.6x** |
| Extreme tiny files (avg 172 B/file) | 962 files/s | 1,743 files/s | **1.8x** |

Inverted view: per 1 GiB compressed, the large-file scenario takes ~54 s old vs ~7.8 s new; the long-path scenario ~70 s old vs ~19.5 s new.

The pattern: the bigger the files, the bigger the gain — compression/encryption parallelizes across cores, while directory enumeration and per-file I/O are serial endpoints that do not scale with core count.

Because compression relies solely on Huffman coding, the **compression ratio is limited**, with a best-case ratio of about **60%**.

Peak memory is hard-capped by the buffer pool: about `2 x (cores + 2)` 8 MB blocks (~300 MB on a 16-core machine), independent of total input size.

Decompression is still single-threaded at about 20-24 MiB/s.

---

# Safety Notice

Although the program includes:

- a **logical root directory mechanism**
- a **duplicate-path skipping mechanism**

file overwriting **may still occur in extreme situations**.

Please **always back up your original files before using the software**.

Restoring Windows symbolic links requires Developer Mode or the "Create symbolic links" privilege; Junction restoration does not require it.

Link targets are not copied into the archive. On another machine, a missing target becomes a normal dangling link, while an absolute target continues to point to the recorded location.

---

# Project Documentation

The project documentation includes:

- `instructions.md`
  Project instructions and HuffmanZip design documentation

- `策划.md`
  Project planning document

- `开发日志.md`
  Development timeline and technical details

---

# Version History

## v1.0.0 — Preview

A small number of known bugs existed.

The main cause was **incorrect construction of the Huffman tree when handling single-character input**.

---

## v1.0.1 — Stable

Fixed boundary handling issues in Huffman encoding.

Applied lightweight optimization and visual adjustments to the executable.

The compiler optimization level was upgraded from **O2 to O3**.

---

## v1.1.1 — Well Done (CLI)

Encapsulated file I/O operations and several helper methods (such as `seek*` functions).

Fixed the directory block processing issue.

Memory usage is stable around **60 MB**.

---

## v2.0.0 — GUI Release (2026-04-16)

**New Features**:
- Qt 6 graphical user interface
- Left-right two-column layout
- Drag-and-drop support
- Real-time progress and log output
- Minimal deployment (reduced by ~30MB)

---

## v2.1.0 — Strategy (2026-04-16)

**New Features**:
- Strategy pattern refactoring: 4 compression/encryption modes
  - Huffman + AES (backward compatible with legacy .sy files)
  - Huffman Only (default, compression only, no encryption)
  - AES Only (encryption only, no compression)
  - Pack Only (archive only, no compression or encryption)
- Auto-detection on decompression via header strategy field
- GUI mode selector on compression tab
- Subfolder name input and reset button on decompression tab
- **Qt 6.2.4 static linking**: package size reduced from 57MB to 16MB (single exe, no DLL dependencies)
- Background image optimized: PNG → JPEG, embedded resource reduced from 11MB to 1.8MB
- Fixed Chinese character path garbling during decompression

**Architecture Changes**:
- Added `NullCompression` / `NullEncryption` null implementations
- Added `StrategyFactory`
- Refactored `CompressionLoop` / `DecompressionLoop` / `BinaryStandardLoader` to use interface references (`ICompression&` / `IEncryption&`)
- Header `strategy` field now in use
- Added `USE_STATIC_QT` CMake toggle for static/dynamic linking
- LGPL v3 compliance notice

---

## v2.2.0 — Pipeline (2026-09-07)

**Multithreaded compression pipeline**: compression now runs as a 3-stage pipeline — dedicated reader thread → N compute workers → dedicated writer thread (N = logical CPU cores), with a buffer pool capping memory usage.

Measured compression throughput (HuffmanAES, 16 logical cores, same machine and identical input; see DevFiles design doc, section 5.1):

| Data profile | Old throughput | New throughput | Speedup |
|---|---|---|---|
| Large-file set 4GiB (3,871 files) | 19.1 MiB/s | 131.9 MiB/s | **6.9x** |
| Long-path full set 2.4GiB (33,835 files, up to 267 chars) | 14.7 MiB/s | 52.6 MiB/s | **3.6x** |
| Extreme tiny files, 150k of them (41 MiB total) | 962 files/s | 1,743 files/s | **1.8x** |

Full-size reference: a 13.47GiB project tree (45,390 files) compresses at 106.5 MiB/s.

**Performance fixes**:
- AES module: CSP handle caching (previously acquired/released a system handle on every encrypt — heavy and contended under threads)
- Write path split into 3 phases: header build → pure-append data write → finalizer for unified backfill/encryption; block lengths written directly (two seek-backs per block removed)

**Architecture changes**:
- Added `ThreadPool/SafeQueue.h` (monitor-style blocking queue) and `ThreadPool/WriteSorter.h` (out-of-order result resequencing), each with unit tests
- `compressionLoop` signature is now `(paths, mode, password)`; module assembly moved into worker threads (one private AES/Huffman instance per worker)
- Runtime types split into `RuntimeLibrary.h` (`Y_flib::Runtime`); `FileLibrary.h` keeps on-disk format types only
- Archive byte layout is identical to the previous version (byte-exact acceptance via `tool_archivebaseline` on non-encrypted modes)

**Compatibility**: v1/v2 archive reading unchanged; HuffmanAES output stays backward compatible.

---

# Build Instructions

## CLI Build

To compile the CLI version yourself:

### 1. Download Build Configuration

Download the `.vscode` archive from the Release page.

---

### 2. Place in Project Directory

Place the extracted `.vscode` folder into:

```
Y_Manager/
```

---

### 3. Compile the Program

Compile `main.cpp` to generate the executable.

---

## GUI Build

The GUI version is in the `SFC_GUI/` directory and requires Qt 6.2.4 LTS.

### Requirements

- **Qt 6.2.4 LTS** (with bundled MinGW 11.2.0)
- **CMake 3.20+**
- **Static linking mode**: requires pre-built Qt 6.2.4 static libraries (`D:/qt/6.2.4-static-mingw/`)

### Build Steps (Static)

Use the `build_static.bat` script (recommended):

```bash
# Run in cmd.exe
cd SFC_GUI
build_static.bat
```

Or build manually:

```bash
cd SFC_GUI
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DUSE_STATIC_QT=ON
cmake --build build --parallel
```

After building, a single exe is in the `bin/SFC/` directory with no DLL dependencies.

### Build Steps (Dynamic)

```bash
cd SFC_GUI
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DUSE_STATIC_QT=OFF
cmake --build build --config Release -j 8
```

After building, the executable is in the `bin/SFC/` directory.

---

# System Requirements

## CLI Requirements

- **OS**: Windows 10 or later
- **Architecture**: x64

## GUI Requirements

- **OS**: Windows 10 1607+ (static build) / Windows 10 1809+ (dynamic build)
- **Architecture**: x64
- **Package size**: 16MB static (single exe) / 57MB dynamic (exe + DLLs + plugins)

---

### Compilation Requirements

- Use the **C++20 standard**

- Compiler option:

```
-std=c++20
```

- Recommended optimization level:

```
-O3
```

- **Do not enable LTO (Link Time Optimization)**

- **The GUI version must use Qt's bundled MinGW**

---

# Dependencies

## CLI Dependencies

- **OpenSSL**
  - `SHA256`
  - `RAND`

- **Windows 10–11 API**
  Used for character encoding control and rand()

## GUI Dependencies

- **Qt 6.2.4 LTS** (Core, Widgets)
- **MinGW 11.2.0** (Qt bundled)
- **Windows 10–11 API**
- **Additional dependencies for static linking mode**:
  - Qt 6.2.4 static libraries (`D:/qt/6.2.4-static-mingw/`)
  - Windows system libraries: dwmapi, uxtheme, imm32, oleaut32, version, setupapi, fontsub

---

# Disclaimer

This project is intended **for educational and learning purposes only**.

Please **back up your original files before using the software**.
