# Simple Files Compressor — Agent 入口

本项目知识的唯一入口是 `Y_MultipleAgentWorkflow/Router.md`。本文件只声明入口与铁律，
不索引任何下层文档。需要什么，由根 Router 逐步路由过去。

## 入口

| 起点 | 位置 |
|---|---|
| 唯一权威入口 | `Y_MultipleAgentWorkflow/Router.md` |

## 铁律

1. **先路由再动手。** 任何任务从根 Router 起步，由它决定读哪一篇。不凭记忆或本文件
   推断项目事实。
2. **权威顺序。** 用户最新明确要求 > 实际代码与可复现结果 > 业务 Guide/Design > 业务
   Router > DeveloperLog 与 Proposal > 模型入口与 Skill。文档与代码冲突时以代码为准，
   并在同一次修改中修正文档。
3. **写前取租约。** 修改文件、派生 subagent、执行会改变状态的命令之前，按根 Router 指向
   的并发指南取得 WorkingAgent 租约；路径重叠即冲突。
4. **单一权威。** 同一个事实只在一篇文档里定义，别处只写引用。
5. **以工作流为准。** 项目知识以 `Y_MultipleAgentWorkflow/` 为准，`DevFiles/` 只作背景
   资料。

## 不要做

- 不要把本文件当索引用。新增文档只在对应 Router 登记，不在这里加行。
- 不要新建顶级业务分类；确有必要时先取得用户确认。
- 不要在 `Y_MultipleAgentWorkflow/` 里留临时草稿、占位符或未核验的数字。
