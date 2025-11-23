# We_compress

这是一个用于压缩和解压文件的开源工具，支持多种压缩算法，提供简单易用的命令行接口和高效的压缩性能。

## 功能特性

- 支持多种压缩格式（如 ZIP, GZIP, TAR 等）
- 提供命令行工具进行文件压缩与解压
- 高效的压缩算法，节省存储空间
- 支持跨平台使用（Windows, Linux, macOS）

## 安装

### 使用命令行安装

确保你已经安装了 `git` 和 `cargo`（Rust 编译工具链）：

```bash
# 克隆仓库
git clone https://gitee.com/fifseason/We_compress.git
cd We_compress

# 编译并安装
cargo build --release
cargo install --path .
```

### 使用包管理器安装（待补充）

目前暂不支持包管理器安装，后续版本将提供相关支持。

## 使用方法

### 压缩文件

```bash
we_compress compress <input_file> <output_file>
```

### 解压文件

```bash
we_compress decompress <input_file> <output_file>
```

### 查看帮助

```bash
we_compress --help
```

## 贡献指南

欢迎贡献代码、提交 issue 或提出改进建议。请遵循以下步骤：

1. Fork 仓库
2. 创建新分支 (`git checkout -b feature/new-feature`)
3. 提交更改 (`git commit -m 'Add new feature'`)
4. 推送分支 (`git push origin feature/new-feature`)
5. 提交 Pull Request

## 许可证

本项目采用 MIT 许可证。详情请查看 [LICENSE](LICENSE) 文件。