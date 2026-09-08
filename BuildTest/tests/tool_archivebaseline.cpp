// tool_archivebaseline.cpp — 归档基准生成/对比 CLI 工具（零外部依赖，仿 test_flagtype 模式）
//
//   tool_archivebaseline sample    <root>                    生成固定样本集（root/sample 树 + root/extra.txt）
//   tool_archivebaseline compress  <root> <out.sy> <mode>    压缩样本集（mode: pack | huffman，均无加密）
//   tool_archivebaseline decompress <in.sy> <outdir>         解压归档（outdir 必须不存在）
//   tool_archivebaseline roundtrip <root> <mode>             压缩→解压→逐文件校验，一条命令完成往返验收
//   tool_archivebaseline verify    <dirA> <dirB>             递归对比两棵树（文件集合 + 逐字节）
// 退出码：0 成功，非 0 失败（可直接用于脚本判定）。
//
// 路径对应关系：compress 扫描 {root/sample, root/extra.txt}，logicalRoot 固定为 "baseline_root"，
// 解压恢复为 outdir/baseline_root/sample/... 与 outdir/baseline_root/extra.txt。
//
// 确定性说明：样本内容由固定种子 LCG 生成；Windows/NTFS 的 directory_iterator
// 在同一目录上顺序稳定，因此同机生成的归档逐字节可复现（加密模式因 IV 随机不适用，本工具只做无加密模式）。

#include "FileLibrary.h"
#include "EncodingUtils.h"
#include "FileSystemUtils.h"
#include "StrategyFactory.h"
#include "HeaderWriter.h"
#include "MainLoop.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// UTF-8 字面量路径辅助（样本树固定，直接内嵌 u8 字面量）
static fs::path u8p(const char8_t *p) { return fs::path(p); }

// 固定种子 LCG：同一调用序列产生完全相同的字节流
static uint32_t lcgState = 12345u;
static unsigned char nextByte()
{
    lcgState = lcgState * 1664525u + 1013904223u;
    return static_cast<unsigned char>(lcgState >> 24);
}

static void writeLcgFile(const fs::path &path, uint64_t size, uint32_t seed)
{
    std::ofstream out(
        Y_flib::FileSystemUtils::pathForIo(path),
        std::ios::binary | std::ios::trunc);
    if (!out)
        throw std::runtime_error("sample: cannot create " + Y_flib::EncodingUtils::pathToUtf8(path));
    lcgState = seed;
    const size_t CHUNK = 64 * 1024;
    std::vector<unsigned char> buf;
    for (uint64_t written = 0; written < size;)
    {
        uint64_t n = std::min<uint64_t>(CHUNK, size - written);
        buf.resize(n);
        for (auto &b : buf)
            b = nextByte();
        out.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(n));
        written += n;
    }
}

static void writeTextFile(const fs::path &path, const std::string &text)
{
    std::ofstream out(
        Y_flib::FileSystemUtils::pathForIo(path),
        std::ios::binary | std::ios::trunc);
    if (!out)
        throw std::runtime_error("sample: cannot create " + Y_flib::EncodingUtils::pathToUtf8(path));
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
}

static int makeSample(const std::string &rootUtf8)
{
    const fs::path root = Y_flib::EncodingUtils::pathFromUtf8(rootUtf8);
    const fs::path tree = root / "sample";
    std::error_code ec;
    // 回归目录本身包含超长路径，需使用兼容层清理才能稳定重复执行。
    Y_flib::FileSystemUtils::removeAll(root);
    fs::create_directories(tree / u8p(u8"测试目录") / u8p(u8"嵌套一") / u8p(u8"深"), ec);
    fs::create_directories(tree / "emptyDir", ec); // 空目录：覆盖目录恢复路径
    if (ec)
    {
        std::cerr << "sample: create_directories failed\n";
        return 1;
    }

    writeTextFile(tree / "readme.txt",
                  "Simple Files Compressor - archive baseline sample.\n"
                  "This tree is deterministic (fixed-seed LCG) for byte-exact comparison.\n");
    writeLcgFile(tree / "empty.bin", 0, 1);    // 0 字节文件
    writeLcgFile(tree / "small.bin", 1024, 2); // 1KB
    writeLcgFile(tree / "medium.bin", 100 * 1024, 3);
    writeLcgFile(tree / "blockedge.bin", 8ull * 1024 * 1024 + 1, 4); // 覆盖 8MB 读取块边界
    writeTextFile(tree / u8p(u8"测试目录") / u8p(u8"说明.txt"),
                  "中文名目录下的文本文件，验证 UTF-8 路径的往返一致性。\n");
    writeLcgFile(tree / u8p(u8"测试目录") / u8p(u8"数据.bin"), 64 * 1024, 5);
    writeLcgFile(tree / u8p(u8"测试目录") / u8p(u8"嵌套一") / u8p(u8"深") / "deep_file.bin",
                 3 * 1024 + 7, 6);

    // 样本树之外的独立散文件：构成第二个扫描条目，覆盖多路径输入
    writeLcgFile(root / "extra.txt", 5 * 1024 + 29, 7);

    // 多文件目录：500 个小文件让目录区标准累计超过 HEADER_BUFFER_SIZE(16KB)，
    // 强制触发目录区分割标准的回填/预留路径（这是目录区写入最关键的分支部）
    {
        const fs::path many = tree / "manyfiles";
        fs::create_directories(many, ec);
        for (int i = 0; i < 500; ++i)
        {
            std::string name = "f" + std::to_string(i) + ".bin";
            writeLcgFile(many / name, 80 + (i % 64), 100 + static_cast<uint32_t>(i));
        }
    }

    // 深层包目录树（pnpm 形态）：0pkg 前缀使其按字母序排在所有文件条目之前，
    // 于是首个 16KB 目录块内只有目录条目、没有任何文件条目——覆盖
    // "CompressionLoop 启动时 fileQueue 为空" 的回归场景（曾静默产出无数据区的残缺归档）
    {
        uint32_t deepSeed = 500;
        for (int i = 0; i < 100; ++i)
        {
            lcgState = deepSeed + static_cast<uint32_t>(i);
            std::string suffix;
            for (int k = 0; k < 32; ++k)
                suffix.push_back(static_cast<char>('a' + (nextByte() % 26)));
            const std::string pkg = "0pkg-" + std::to_string(i) + "@1.0_dep@2.0-" + suffix;
            const fs::path base = tree / fs::path(pkg) / "node_modules" / "@scope" / ("pkg-" + std::to_string(i));
            fs::create_directories(base / "lib", ec);
            fs::create_directories(base / "data", ec);
            writeLcgFile(base / "lib" / "a.js", 40 + (i % 7), deepSeed + 1000 + static_cast<uint32_t>(i));
            writeLcgFile(base / "data" / "b.json", 40 + (i % 5), deepSeed + 2000 + static_cast<uint32_t>(i));
        }
    }

    // 让短文件仍低于 MAX_PATH，而 package.json 的完整路径越过 260。
    // 旧实现会把目录子项数写成 2，却因 MinGW status 失败只序列化短文件，
    // 从而让后续 BFS 父子关系整体错位。
    {
        fs::path longParent = tree / "long-path";
        constexpr size_t targetParentLength = 250;
        while (longParent.native().size() < targetParentLength)
        {
            const size_t remaining =
                targetParentLength - longParent.native().size() - 1;
            if (remaining == 0)
                break;
            longParent /= std::string(std::min<size_t>(remaining, 120), 'x');
        }

        Y_flib::FileSystemUtils::createDirectories(longParent);
        writeTextFile(longParent / "a", "short path sibling\n");
        writeTextFile(longParent / "package.json",
                      "{\"name\":\"long-path-regression\"}\n");

        const fs::path deeperParent = longParent / "nested-child";
        Y_flib::FileSystemUtils::createDirectories(deeperParent);
        writeTextFile(deeperParent / "inside.txt",
                      "directory enumeration beyond MAX_PATH\n");
    }

    std::cout << "sample: tree written to " << rootUtf8 << "\n";
    return 0;
}

static int doCompress(const std::string &rootUtf8, const std::string &outUtf8, const std::string &mode)
{
    Y_flib::CompressionMode m;
    if (mode == "pack")
        m = Y_flib::CompressionMode::PackOnly;
    else if (mode == "huffman")
        m = Y_flib::CompressionMode::HuffmanOnly;
    else
    {
        std::cerr << "compress: unknown mode '" << mode << "' (expected pack|huffman)\n";
        return 1;
    }

    const fs::path outPath = Y_flib::EncodingUtils::pathFromUtf8(outUtf8);
    const fs::path root = Y_flib::EncodingUtils::pathFromUtf8(rootUtf8);
    std::error_code ec;
    fs::remove(outPath, ec); // HeaderWriter 要求输出文件不存在

    // 两个扫描条目：样本目录 + 样本树外的独立散文件，覆盖 GUI 常见的多路径输入
    std::vector<std::string> filePathToScan = {
        Y_flib::EncodingUtils::pathToUtf8(root / "sample"),
        Y_flib::EncodingUtils::pathToUtf8(root / "extra.txt")};

    // 三阶段写入的第一阶段：建档（写文件头 + 目录树 + 各预留字段），与 GUI CompressionWorker 相同的顺序
    std::string outPathUtf8 = outUtf8; // headerWriter 形参为非 const 引用
    Y_flib::HeaderWriter headerWriter;
    headerWriter.headerWriter(filePathToScan, outPathUtf8, "baseline_root", m);

    CompressionLoop compressor(outUtf8);
    compressor.compressionLoop(filePathToScan, m, ""); // 流水线内部按 (mode, password) 自制模块

    std::cout << "compress: " << outUtf8 << " (" << mode << ", "
              << fs::file_size(outPath) << " bytes)\n";
    return 0;
}

static int doDecompress(const std::string &inUtf8, const std::string &outDirUtf8)
{
    const fs::path outDir = Y_flib::EncodingUtils::pathFromUtf8(outDirUtf8);
    std::error_code ec;
    if (fs::exists(outDir, ec))
    {
        std::cerr << "decompress: output directory must not exist: " << outDirUtf8 << "\n";
        return 1;
    }
    fs::create_directories(outDir, ec);
    if (ec)
    {
        std::cerr << "decompress: create output directory failed\n";
        return 1;
    }

    // 从归档头识别策略并组装模块（与 GUI CompressionWorker 相同的协议）
    Y_flib::Header fileHeader;
    {
        std::ifstream in(Y_flib::EncodingUtils::pathFromUtf8(inUtf8), std::ios::binary);
        if (!in)
        {
            std::cerr << "decompress: cannot open archive\n";
            return 1;
        }
        std::vector<unsigned char> buf(Y_flib::Constants::HEADER_SIZE);
        in.read(reinterpret_cast<char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
        std::memcpy(&fileHeader, buf.data(), sizeof(fileHeader));
        if (fileHeader.magicNum_1 != Y_flib::Constants::MAGIC_NUM)
        {
            std::cerr << "decompress: bad magic\n";
            return 1;
        }
    }
    const Y_flib::CompressionMode detected = Y_flib::StrategyFactory::idToMode(fileHeader.strategy);

    DecompressionLoop decompressor(inUtf8, outDirUtf8);
    decompressor.decompressionLoop(detected, ""); // 流水线内部按 (mode, password) 自制模块

    std::cout << "decompress: done -> " << outDirUtf8 << "\n";
    return 0;
}

// 收集一棵树的文件清单：相对路径(UTF-8, 斜杠分隔) -> 绝对路径
static std::map<std::string, fs::path> collectFiles(const fs::path &root)
{
    std::map<std::string, fs::path> files;
    std::vector<fs::path> directories{root};
    while (!directories.empty())
    {
        const fs::path directory = directories.back();
        directories.pop_back();

        for (const fs::path &fullPath :
             Y_flib::FileSystemUtils::listDirectory(directory))
        {
            const Y_flib::FileSystemEntryInfo info =
                Y_flib::FileSystemUtils::queryEntry(fullPath);
            if (info.isRegularFile)
            {
                const fs::path rel = fullPath.lexically_relative(root);
                files[Y_flib::EncodingUtils::pathToUtf8(rel)] = fullPath;
            }
            else if (info.isDirectory)
            {
                directories.push_back(fullPath);
            }
        }
    }
    return files;
}

static bool filesByteEqual(const fs::path &a, const fs::path &b)
{
    const Y_flib::FileSystemEntryInfo infoA =
        Y_flib::FileSystemUtils::queryEntry(a);
    const Y_flib::FileSystemEntryInfo infoB =
        Y_flib::FileSystemUtils::queryEntry(b);
    if (!infoA.isRegularFile || !infoB.isRegularFile || infoA.size != infoB.size)
        return false;
    std::ifstream fa(Y_flib::FileSystemUtils::pathForIo(a), std::ios::binary);
    std::ifstream fb(Y_flib::FileSystemUtils::pathForIo(b), std::ios::binary);
    if (!fa || !fb)
        return false;
    std::vector<unsigned char> ba(64 * 1024), bb(64 * 1024);
    while (fa && fb)
    {
        fa.read(reinterpret_cast<char *>(ba.data()), static_cast<std::streamsize>(ba.size()));
        fb.read(reinterpret_cast<char *>(bb.data()), static_cast<std::streamsize>(bb.size()));
        if (fa.gcount() != fb.gcount() ||
            std::memcmp(ba.data(), bb.data(), static_cast<size_t>(fa.gcount())) != 0)
            return false;
    }
    return true;
}

static int doVerify(const std::string &aUtf8, const std::string &bUtf8)
{
    const fs::path rootA = Y_flib::EncodingUtils::pathFromUtf8(aUtf8);
    const fs::path rootB = Y_flib::EncodingUtils::pathFromUtf8(bUtf8);
    auto filesA = collectFiles(rootA);
    auto filesB = collectFiles(rootB);

    bool ok = true;
    for (const auto &[rel, pathA] : filesA)
    {
        auto it = filesB.find(rel);
        if (it == filesB.end())
        {
            std::cerr << "verify: only in A: " << rel << "\n";
            ok = false;
            continue;
        }
        if (!filesByteEqual(pathA, it->second))
        {
            std::cerr << "verify: content differs: " << rel << "\n";
            ok = false;
        }
    }
    for (const auto &[rel, pathB] : filesB)
    {
        if (filesA.find(rel) == filesA.end())
        {
            std::cerr << "verify: only in B: " << rel << "\n";
            ok = false;
        }
    }

    if (ok)
        std::cout << "verify: PASS (" << filesA.size() << " files identical)\n";
    else
        std::cout << "verify: FAIL\n";
    return ok ? 0 : 1;
}

static int doCompressDir(const std::string &dirUtf8, const std::string &outUtf8, const std::string &mode)
{
    Y_flib::CompressionMode m;
    if (mode == "pack")
        m = Y_flib::CompressionMode::PackOnly;
    else if (mode == "huffman")
        m = Y_flib::CompressionMode::HuffmanOnly;
    else
    {
        std::cerr << "compressdir: unknown mode '" << mode << "' (expected pack|huffman)\n";
        return 1;
    }

    const fs::path outPath = Y_flib::EncodingUtils::pathFromUtf8(outUtf8);
    std::error_code ec;
    fs::remove(outPath, ec);

    std::vector<std::string> filePathToScan = {dirUtf8};

    std::string outPathUtf8 = outUtf8;
    Y_flib::HeaderWriter headerWriter;
    headerWriter.headerWriter(filePathToScan, outPathUtf8, "baseline_root", m);

    CompressionLoop compressor(outUtf8);
    compressor.compressionLoop(filePathToScan, m, ""); // 流水线内部按 (mode, password) 自制模块

    std::cout << "compressdir: " << outUtf8 << " (" << mode << ", "
              << fs::file_size(outPath) << " bytes)\n";
    return 0;
}

// 一条命令完成往返验收：压缩→解压→按 compress 的路径对应关系逐文件校验
static int doRoundtrip(const std::string &rootUtf8, const std::string &mode)
{
    const fs::path root = Y_flib::EncodingUtils::pathFromUtf8(rootUtf8);
    const fs::path work = root / ("roundtrip-" + mode);
    std::error_code ec;
    Y_flib::FileSystemUtils::removeAll(work);
    fs::create_directories(work, ec);

    const std::string archive = Y_flib::EncodingUtils::pathToUtf8(work / "baseline.sy");
    const std::string outDir = Y_flib::EncodingUtils::pathToUtf8(work / "out");

    if (doCompress(rootUtf8, archive, mode) != 0)
        return 1;
    if (doDecompress(archive, outDir) != 0)
        return 1;

    const fs::path restored = Y_flib::EncodingUtils::pathFromUtf8(outDir) / "baseline_root";
    bool ok = doVerify(Y_flib::EncodingUtils::pathToUtf8(root / "sample"),
                       Y_flib::EncodingUtils::pathToUtf8(restored / "sample")) == 0;

    if (!fs::exists(restored / "extra.txt", ec))
    {
        std::cerr << "roundtrip: extra.txt missing after decompress\n";
        ok = false;
    }
    else if (!filesByteEqual(root / "extra.txt", restored / "extra.txt"))
    {
        std::cerr << "roundtrip: extra.txt content differs\n";
        ok = false;
    }

    std::cout << "roundtrip(" << mode << "): " << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}

int main(int argc, char **argv)
{
    try
    {
        if (argc == 3 && std::string(argv[1]) == "sample")
            return makeSample(argv[2]);
        if (argc == 5 && std::string(argv[1]) == "compress")
            return doCompress(argv[2], argv[3], argv[4]);
        if (argc == 5 && std::string(argv[1]) == "compressdir")
            return doCompressDir(argv[2], argv[3], argv[4]);
        if (argc == 4 && std::string(argv[1]) == "decompress")
            return doDecompress(argv[2], argv[3]);
        if (argc == 4 && std::string(argv[1]) == "roundtrip")
            return doRoundtrip(argv[2], argv[3]);
        if (argc == 4 && std::string(argv[1]) == "verify")
            return doVerify(argv[2], argv[3]);
    }
    catch (const std::exception &e)
    {
        std::cerr << "tool_archivebaseline: " << e.what() << "\n";
        return 1;
    }

    std::cerr << "usage:\n"
              << "  tool_archivebaseline sample <root>\n"
              << "  tool_archivebaseline compress <root> <out.sy> <pack|huffman>\n"
              << "  tool_archivebaseline compressdir <dir> <out.sy> <pack|huffman>  (arbitrary directory)\n"
              << "  tool_archivebaseline decompress <in.sy> <outdir>\n"
              << "  tool_archivebaseline roundtrip <root> <pack|huffman>\n"
              << "  tool_archivebaseline verify <dirA> <dirB>\n";
    return 1;
}
