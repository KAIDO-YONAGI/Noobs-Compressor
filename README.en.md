# SFC — Simple Files Compressor

English | [简体中文](README.md)

SFC is a graphical archiving tool for Windows. It collects files and directories into a single
`.sy` archive, compresses it in 8 MiB blocks with Huffman coding, encrypts data and directory
blocks with AES-128-CTR when the mode calls for it, and restores the recorded directory structure
on extraction, symbolic links and junctions included.

Version 2.2.1, archive format v3.

## 1. Capabilities

| Capability | Notes |
|---|---|
| Multiple inputs | Several files and several directories in one task; a whole folder can be dropped in |
| Four processing modes | Huffman, Huffman + AES, AES and Pack combinations |
| Three-stage pipeline | One reader thread, N compute workers and one writer thread; N defaults to the logical CPU count |
| Memory independent of input size | Peak memory tracks worker count and block size |
| Single logical root | Every entry sits under one root directory named after the archive; entry names are stored as relative paths |
| Input de-duplication | A full path already in the list is not added again |
| Windows links | Symbolic links and junctions record type, name and target; scanning does not follow them |
| GUI | Drag and drop, live progress, per-file log, Chinese translation |

## 2. Processing modes

The mode is chosen in the right column of the compression page; Huffman Only is the default. The
mode number goes into the `strategy` byte of the header, and extraction picks its read path from
that byte.

| Mode | `strategy` | Compression | Encryption |
|---|---|---|---|
| Huffman + AES | 0 | Huffman | AES-128-CTR |
| Huffman Only | 1 | Huffman | none |
| AES Only | 2 | none | AES-128-CTR |
| Pack Only | 3 | none | none |

Huffman + AES and AES Only require a passphrase before writing starts. Encryption covers data
blocks and directory blocks; Pack Only and Huffman Only keep the directory region in plaintext.

## 3. Archive layout

```text
[ 19-byte header ][ directory region ][ directory tail magic ][ data region ]
```

- Header: opening magic, `strategy`, `version`, header length, directory offset, second magic.
- Directory region: a chain of directory blocks, each preceded by a 25-byte separator standard,
  with a new block starting once the current body reaches 16 KiB. A block holds directory, file,
  logical-root, symbolic-link and junction entries.
- Directory tail magic: 4 bytes. The directory offset counts it in, and the data region starts at
  that offset.
- Data region: each source file is split into 8 MiB blocks, and every block is written as a
  metadata block followed by a data block. Huffman metadata is a 258-byte code-length table; a
  data block body starts with one byte holding the padding count, then the code stream. In AES
  modes the first 16 bytes of every block body are that block's random IV.

The reader accepts archives whose `version` is 3 and verifies all three magic values.

The byte-level specification, offset arithmetic and a minimal worked example are in
[the archive format specification](Y_MultipleAgentWorkflow/ArchiveFormat/归档格式.md).

## 4. Usage

### Compression

1. Add files and directories to the list with the left-column buttons or by dragging them in.
2. Check the output directory and output filename. Both are filled in after the first entry is
   added and can be edited.
3. Pick a mode in the right column. Encrypted modes need a passphrase.
4. Click **Start Compression**. The archive is written to the output directory with the `.sy`
   extension.

### Extraction

1. Select a `.sy` archive, or drag the file into the window.
2. Choose the output directory. A value in **Subfolder Name** becomes the final root directory
   under the output directory.
3. An encrypted archive needs the same passphrase used at compression time; the task does not
   start with an empty passphrase.
4. Click **Start Decompression**. The archive's logical root directory is created under the root
   from the previous step.

### At runtime

The right column shows overall progress, current-file progress and a line-by-line log capped at
the most recent 500 lines. Closing the window during a task requests a stop; the main loop exits
between stages, and files already written stay in the target directory.

### Environment variables

| Variable | Effect |
|---|---|
| `SFC_WORKERS` | Number of compute workers, a positive integer. Defaults to the hardware concurrency, or 4 when that is unavailable; compression and extraction share the rule |
| `SFC_GUI_LANGUAGE` | UI language such as `zh_CN`; `system` follows the system setting |

UI strings are authored in English. The Chinese translation loads when the preferred language is
Chinese, and the English source text shows otherwise.

## 5. Performance

End-to-end figures come from a real project tree of 13.47 GiB / 45,390 files, Huffman + AES mode,
the default 16 workers, and the complete read-compute-write path including disk I/O:

| Direction | Throughput |
|---|---|
| Compression | 254.3 MiB/s |
| Decompression | 325.1 MiB/s |

Kernel figures are single-threaded, 8 MiB per block, best of repeated runs:

| Stage | Throughput |
|---|---|
| Huffman encode | 471.1 MiB/s |
| Huffman decode | 236.8 MiB/s |
| AES encrypt | 1261.5 MiB/s |
| AES decrypt | 1279.6 MiB/s |

The two sets measure different things and do not convert into each other. The hardware is an AMD
Ryzen 7 8845H with 8 physical cores and 16 logical processors; the build uses `-O3 -DNDEBUG` with
LTO. End-to-end throughput is bounded by the single reader thread and the single writer thread,
while the compute stage still has headroom across cores.

Peak memory is about `2 × (N + 2)` pool blocks plus `2 × N` private worker blocks of 8 MiB each.
A full compression run with 16 workers peaks near 500 MB, independent of total input size.

Compression ratio follows the input: a highly compressible 2,048 MiB sample archives to 896 MiB,
while a near-incompressible real project tree stays at its original size.

Measurement conditions and bottleneck analysis are in
[engineering practice](Y_MultipleAgentWorkflow/Engineering/工程实践.md).

## 6. Building

### Requirements

- Qt 6.2.4 LTS, Core and Widgets components
- The MinGW 11.2.0 toolchain shipped with Qt
- CMake 3.20 or newer
- A prebuilt Qt 6.2.4 static library for static builds

Put the Qt MinGW 11.2.0 `bin` directory first on `PATH` before configuring, and check the Qt
install paths at the top of `BuildTest/CMakeLists.txt`.

### Static linking, the default build

```powershell
cmake -S BuildTest -B BuildTest/build -DCMAKE_BUILD_TYPE=Release -DUSE_STATIC_QT=ON
cmake --build BuildTest/build --parallel
```

The result lands in `BuildTest/bin/SFC/` as one executable with no shipped DLLs.

### Dynamic linking

```powershell
cmake -S BuildTest -B BuildTest/build -DCMAKE_BUILD_TYPE=Release -DUSE_STATIC_QT=OFF
cmake --build BuildTest/build --parallel
```

The Post-Build step runs `windeployqt` to deploy the Qt DLLs and the `platforms` plugin, copies the
MinGW runtime DLLs, and removes files the build does not use, such as network, TLS and style
plugins.

### Tests

```powershell
cmake -S BuildTest/tests -B BuildTest/tests/build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build BuildTest/tests/build --parallel
ctest --test-dir BuildTest/tests/build --output-on-failure
```

The test project and the GUI project configure and build separately and share the core source list
in `BuildTest/sfc_core.cmake`. The suite holds six unit tests and three end-to-end round-trip
cases; `ctest` generates the sample set at run time.

### Compiler settings

C++20; Release adds `-O3 -DNDEBUG`; LTO is on, and CMake selects the toolchain's `gcc-ar` and
`gcc-ranlib` as the archiver.

## 7. Constraints and risks

| Item | Fact |
|---|---|
| Key derivation | The passphrase is hashed with SHA-256 into a 32-byte digest and the first 16 bytes become the AES-128 key, with no salt and a single iteration |
| Integrity | Archives carry no authentication tag. After a bit flip or a rewrite, extraction still completes and yields data that differs from the original |
| Plaintext surface | The header, block count, per-block lengths and total archive size stay plaintext and reveal the original scale; in AES modes the IVs are also prepended in the clear |
| Passphrase strength | Real protection equals passphrase strength, and offline dictionary attacks are cheap |
| Archive origin | Extraction joins archive paths straight onto the output directory, so `..` and absolute paths take effect. Extract only archives you produced yourself |
| Existing files | When the target already holds a file of the same name, data is appended to its end. Start extraction with an empty target directory |
| Symbolic links | Creating them needs Developer Mode or the create-symbolic-link privilege |
| Junctions | Written through a reparse point and need no extra privilege; a relative target is resolved against the link's parent directory, and UNC targets are rejected |
| Link targets | Restored exactly as recorded and never copied with the archive. After extraction on another machine, a missing target yields an ordinary dangling link |
| Commit | Extraction writes final paths directly; a stopped task leaves the files already written in the target directory |

## 8. System requirements

- Windows 10 x64 or newer, x64 architecture
- Static builds need no third-party runtime; dynamic builds need the Qt runtime

## 9. Licence

- This project's own code is GPL-3.0; the full text is in [LICENSE](LICENSE).
- The third-party component is Qt 6.2.4 under LGPL v3, statically linked. Compliance information
  and asset attribution are registered in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## 10. Documentation

The single entry point for all authoritative project documentation is
[`Y_MultipleAgentWorkflow/Router.md`](Y_MultipleAgentWorkflow/Router.md). It routes a reader to the
architecture, archive format, pipeline, codec kernels, GUI and engineering documents, and states
the authority order and maintenance rules.
