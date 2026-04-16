SFC使用说明
SFC User Guide

基本操作 | Basic Operations

添加文件 | Add Files
点击按钮从本地设备中选择文件。支持多选：

按住 Shift 键可选中连续的多个文件
按住 Ctrl（Windows）/ Command（Mac）键可选中不连续的文件
Click to select files from your local device. Multi-selection is supported:

Hold Shift to select a consecutive range of files
Hold Ctrl (Windows) or Command (Mac) to select multiple non-consecutive files

拖拽操作 | Drag & Drop
将文件或文件夹直接拖入软件窗口，路径将自动识别并添加到列表。

Drag and drop files or folders directly into the application window to automatically add them to the list.

操作方式 | How to Use

选中一个或多个文件或文件夹，按住鼠标左键拖入列表区域，松开即可完成添加。

Select one or more files or folders, hold the left mouse button, drag them into the list area, and release to add.

支持类型 | Supported Types

支持同时拖入混合内容，如多个文件和文件夹的组合。

Mixed content is supported, such as a combination of multiple files and folders.

自动去重 | Auto Deduplication

系统会自动检测并跳过列表中已存在的重复路径。

The system automatically detects and skips duplicate paths already in the list.

添加文件夹 | Add Folder
选择一个文件夹，将文件夹内的所有文件添加至待处理列表。

Select a folder to add all files within it to the processing queue.

移除选中 | Remove Selected
从列表中移除当前选中的一个或多个项目。

Remove the currently selected item(s) from the list.

清空列表 | Clear All
清空待处理列表，移除所有已添加的项目。

Clear the processing queue and remove all added items.

选择路径 | Browse
指定文件处理完成后的保存位置。

Specify the output directory for processed files.

压缩模式 | Compression Mode
压缩页右列顶部可选择压缩模式：

Huffman + AES：Huffman 压缩 + AES 加密（默认，向后兼容）
Huffman Only：仅 Huffman 压缩，不加密
AES Only：仅 AES 加密，不压缩
Pack Only：仅打包为 .sy 归档，不压缩不加密

Select a compression mode from the dropdown at the top of the right column:

Huffman + AES: Huffman compression + AES encryption (default, backward compatible)
Huffman Only: Huffman compression only, no encryption
AES Only: AES encryption only, no compression
Pack Only: Archive to .sy without compression or encryption

开始处理 | Start
启动任务，按设定规则处理列表中的所有文件。

Start processing all files in the list according to the configured settings.

解压操作 | Decompression

选择归档文件 | Select Archive
点击 Browse 选择 .sy 归档文件，或直接拖拽 .sy 文件到窗口。

Click Browse to select a .sy archive file, or drag and drop a .sy file into the window.

输出设置 | Output Settings
- 输出目录：解压文件保存位置
- 子文件夹名（可选）：填写后将拼接到输出目录后作为最终路径
- 密码：归档时使用的密码（留空使用默认密码）

- Output Directory: Where decompressed files are saved
- Subfolder Name (optional): Appended to the output directory path
- Password: The password used during compression (leave empty for default)

自动检测 | Auto Detection
解压时程序会自动读取归档文件的策略号，选择对应的解压模块，无需手动指定。

The program automatically reads the strategy field from the archive header and selects the appropriate decompression modules. No manual configuration needed.

重置 | Reset
点击 Reset 按钮可一键清空所有已填写的解压参数。

Click the Reset button to clear all decompression fields at once.

操作提示 | Tips
处理前请确认保存路径是否正确，避免文件保存到错误位置。

如需排除部分文件，可先选中目标，再点击"移除选中"。

Verify the output path before processing to avoid saving files to the wrong location.

To exclude specific files, select them first and click "Remove Selected".
