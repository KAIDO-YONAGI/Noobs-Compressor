# Engineering 业务路由

文档 ID：`BUS-ENGINEERING`  
状态：`Active`  
维护计数：`0/5`  
最后更新：`2026-10-26`

## 任务线索

| 线索 | 权威文档 |
|---|---|
| `构建、CMake 选项、MinGW、LTO、gcc-ar、单元测试、ctest、产物位置` | `工程实践.md` |
| `吞吐、内存峰值、瓶颈定位、SFC_WORKERS、块大小` | `工程实践.md` |
| `威胁模型、完整性、篡改检测、路径穿越、覆盖风险` | `工程实践.md` |

## 下级导航

| 子类 | Router |
|---|---|
| 无 | 无 |

## 并发资源

- `workflow:Engineering`
- `path:BuildTest/`
- `path:.vscode/`
- `pipeline:BuildPublishRun`

## 能力边界

本根拥有构建事实、性能数字与安全结论。性能数字必须带测量条件与来源；安全结论变更需用户确认。