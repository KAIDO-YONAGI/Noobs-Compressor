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

Since **Huffman coding and AES encryption are implemented purely in software**, runtime performance is relatively slow.

Because compression relies solely on Huffman coding, the **compression ratio is limited**, with a best-case ratio of about **60%**.

The program uses a **block-processing strategy**, and under normal path length conditions:

- **Peak memory usage is approximately 100 MB**

Test scenario:

- Approximately **13,000 files and directories**

- Total data size around **230 GB**

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
