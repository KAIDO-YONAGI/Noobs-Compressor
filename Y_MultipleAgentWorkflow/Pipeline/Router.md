# Pipeline 业务路由

文档 ID：`BUS-PIPELINE`  
状态：`Active`  
维护计数：`0/5`  
最后更新：`2026-10-26`

## 任务线索

| 线索 | 权威文档 |
|---|---|
| `线程模型、关闭顺序、卡死、丢块、取消、SFC_WORKERS` | `流水线.md` |
| `BufferPool、SafeQueue、WriteSorter 的接口、所有权与误用后果` | `流水线.md` |

## 下级导航

| 子类 | Router |
|---|---|
| 无 | 无 |

## 并发资源

- `workflow:Pipeline`
- `path:Y_Manager/`
- `path:BufferPool/`
- `path:ThreadPool/`

## 能力边界

并发契约的唯一权威在本根。引入新的共享可变状态前必须先更新本篇，否则视为违反流水线契约。