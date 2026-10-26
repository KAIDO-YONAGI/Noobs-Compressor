# Architecture 业务路由

文档 ID：`BUS-ARCHITECTURE`  
状态：`Active`  
维护计数：`0/5`  
最后更新：`2026-10-26`

## 任务线索

| 线索 | 权威文档 |
|---|---|
| `项目有哪些模块、谁依赖谁、一次压缩或解压怎么走完` | `系统与术语.md` |
| `术语含义、两个词是不是一回事、类型名对应哪个中文概念` | `系统与术语.md` |

## 下级导航

| 子类 | Router |
|---|---|
| 无 | 无 |

## 并发资源

- `workflow:Architecture`
- `path:Y_MultipleAgentWorkflow/Architecture/`
- 源码树只读；改动源码属于其它业务根

## 能力边界

本根拥有全局结构与术语。字节布局引用 ArchiveFormat 根；线程细节引用 Pipeline 根；签名以头文件为准，本根不复写接口清单。