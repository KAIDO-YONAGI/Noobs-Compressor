# Codec 业务路由

文档 ID：`BUS-CODEC`  
状态：`Active`  
维护计数：`0/5`  
最后更新：`2026-10-26`

## 任务线索

| 线索 | 权威文档 |
|---|---|
| `Huffman 编解码、码长表、位打包、压缩率、单核吞吐` | `编解码内核.md` |
| `AES、口令、密钥派生、IV、CTR 模式、加密吞吐` | `编解码内核.md` |

## 下级导航

| 子类 | Router |
|---|---|
| 无 | 无 |

## 并发资源

- `workflow:Codec`
- `path:CompressionModules/`
- `path:EncryptionModules/`

## 能力边界

本根拥有两个算法内核的可变状态、边界条件与失败模式。块结构与分割标准引用 ArchiveFormat 根；安全结论引用 Engineering 根。