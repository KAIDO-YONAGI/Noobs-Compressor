# We_compress

An open-source tool for compressing and decompressing files, supporting multiple compression algorithms with a simple and user-friendly command-line interface and high-efficiency compression performance.

## Features

- Supports multiple compression formats (e.g., ZIP, GZIP, TAR, etc.)
- Provides a command-line tool for file compression and decompression
- Efficient compression algorithms to save storage space
- Cross-platform support (Windows, Linux, macOS)

## Installation

### Install via Command Line

Ensure you have `git` and `cargo` (Rust toolchain) installed:

```bash
# Clone the repository
git clone https://gitee.com/fifseason/We_compress.git
cd We_compress

# Build and install
cargo build --release
cargo install --path .
```

### Install via Package Manager (To Be Added)

Package manager installation is not currently supported; future versions will include this feature.

## Usage

### Compress a File

```bash
we_compress compress <input_file> <output_file>
```

### Decompress a File

```bash
we_compress decompress <input_file> <output_file>
```

### View Help

```bash
we_compress --help
```

## Contribution Guidelines

Contributions are welcome! Please submit code, issues, or improvement suggestions following these steps:

1. Fork the repository
2. Create a new branch (`git checkout -b feature/new-feature`)
3. Commit your changes (`git commit -m 'Add new feature'`)
4. Push the branch (`git push origin feature/new-feature`)
5. Submit a Pull Request

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.