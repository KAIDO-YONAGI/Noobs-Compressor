# Simple Files Compressor 多 Agent 工作流路由

文档 ID：`ROOT-ROUTER`  
状态：`Active`  
最后更新：`2026-10-26`

本目录是 SFC 项目的权威文档入口。Agent 可以先做只读探查，但在进入细化分析、
派生 subagent、写入文件或调用会改变状态的工具之前，必须取得 WorkingAgent 租约。

## 起步顺序

1. 从本文件路由。
2. 读 `Workflow\Concurrency_Guide.md`。
3. 用 `Workflow\Scripts\WorkingAgent.ps1` 取得精确租约。
4. 若租约状态显示重叠任务，且客户端支持原生 Agent 通信，先通知该任务再动手。
5. 只读被路由到的 Guide/Design、Router 与必要证据。
6. 成功、失败或取消都在 finally 路径释放租约。

## 权威顺序

1. 用户当前最新的明确要求。
2. 实际代码、资源、运行结果与可复现实验。
3. 业务 Guide/Design。
4. 业务 Router。
5. DeveloperLog 与 Proposal。
6. 模型入口文件与 Skill。

## 业务路由

| 业务根 | Router |
|---|---|
| `Architecture` | `Architecture\Router.md` |
| `ArchiveFormat` | `ArchiveFormat\Router.md` |
| `Codec` | `Codec\Router.md` |
| `Engineering` | `Engineering\Router.md` |
| `GUI` | `GUI\Router.md` |
| `Pipeline` | `Pipeline\Router.md` |
| `Workflow` | `Workflow\Router.md` |

## 项目验证

本项目的构建、测试与状态变更流程，统一走 `Workflow\Project_Validation_Guide.md`。
该指南只收录本仓库中确实存在的命令，不套用其它项目的流程。

## 文档索引

| 文档 ID | 路径 | 状态 | 职责 |
|---|---|---|---|
| `ARCH-ARCH` | `Architecture/系统与术语.md` | Active | 模块分层、依赖方向、端到端数据流、术语表 |
| `FMT-SPEC` | `ArchiveFormat/归档格式.md` | Active | 逐字节规范、四条读写路径、归档路径与链接规则 |
| `PIPE-DESIGN` | `Pipeline/流水线.md` | Active | 三段流水线、序号与重排、关闭协议、并发组件 |
| `CODEC-CORE` | `Codec/编解码内核.md` | Active | Huffman 与 AES 两个内核的规范与边界 |
| `GUI-DESIGN` | `GUI/界面设计.md` | Active | 窗口结构、跨线程信号链、模式状态机 |
| `ENG-GUIDE` | `Engineering/工程实践.md` | Active | 构建与测试、性能画像、威胁模型与残余风险 |
| `WF-DOC-STANDARD` | `Workflow/Documentation_Standard.md` | Active | 文档撰写规则：时态、证据、结构、禁用表述 |
| `WF-GUIDE` | `Workflow/Workflow_Guide.md` | Active | 路由、分类增殖、深度、维护计数 |
| `WF-CONCURRENCY` | `Workflow/Concurrency_Guide.md` | Active | 租约、冲突、心跳、完成释放 |
| `WF-PROJECT-VALIDATION` | `Workflow/Project_Validation_Guide.md` | Reference | 本项目构建与测试命令 |
| `WF-CONFIG-METHOD` | `Workflow_Configuration_Guide.md` | Active | 通用多 Agent 工作流配置方法 |

## 维护

成功且实质影响某个业务的任务，会为该业务根的 `0/5` 计数加一。到达 `5/5` 时，
复查任务证据与项目实际状态，更新权威文档或记录 `reviewed-no-change`，然后归零。
