# 工作流路由

文档 ID：`WF-ROUTER`  
状态：`Active`  
维护计数：`0/5`  
最后更新：`2026-10-26`

本目录存放工作流自身的规则、模板与租约脚本，不存放业务事实。

| 任务 | 该读什么 |
|---|---|
| 路由规则、分类增殖、层级深度、维护计数 | `Workflow_Guide.md` |
| 租约、冲突、等待、覆盖、心跳 | `Concurrency_Guide.md` |
| 文档写作规则：文档头、证据要求、禁用表述 | `Documentation_Standard.md` |
| 本项目的构建与测试命令 | `Project_Validation_Guide.md` |
| 新建业务根的骨架 | `Templates\BusinessRouter.template.md` |
| 新建业务日志的骨架 | `Templates\DeveloperLog.template.md` |
| 租约操作 | `Scripts\WorkingAgent.ps1` |
| 租约回归测试 | `Scripts\Test-WorkingAgent.ps1` |

通用方法论的来源是 `Workflow_Configuration_Guide.md`，它与本目录的规则冲突时，
以本目录的项目实际配置为准。

## 变更约束

- 修改根级分类、工作区级规则或模型入口，必须先获得用户确认。
- 修改脚本与模板属于 managed 文件，改动会被实例清单登记为本地漂移，需在提交说明中写明。
- 本目录不写业务结论；业务结论只写在对应业务根。
