# ArchiveFormat 业务路由

文档 ID：`BUS-ARCHIVEFORMAT`  
状态：`Active`  
维护计数：`0/5`  
最后更新：`2026-10-26`

## 任务线索

| 线索 | 权威文档 |
|---|---|
| `解析或生成 .sy 归档、字段偏移、字节布局、魔数、分割标准、版本闸门` | `归档格式.md` |
| `建档、数据写入、收尾回填、读取解析的实现路径` | `归档格式.md` |
| `归档时路径怎么写、符号链接与 Junction 怎么记、解压怎么恢复` | `归档格式.md` |

## 下级导航

| 子类 | Router |
|---|---|
| 无 | 无 |

## 并发资源

- `workflow:ArchiveFormat`
- `path:CompressorFileSystem/ArchiveFormat/`
- `path:CompressorFileSystem/Commons/include/FileLibrary.h`
- `path:CompressorFileSystem/Commons/include/RuntimeLibrary.h`
- `path:CompressorFileSystem/Commons/src/FileSystemUtils.cpp`
- `path:CompressorFileSystem/Commons/src/EncodingUtils.cpp`

## 能力边界

磁盘格式与归档路径规则的唯一权威在本根。其它文档遇到字节布局与路径记录一律引用。改动字段宽度必须同时更新布局结构体、static_assert 与本篇。