# WorkingAgent 租约注册表

文档 ID：`WF-WORKING-AGENT`  
状态：`Active`  
最后更新：`2026-10-26`

本目录存放临时的 JSON 租约标记，文件名为 `<AgentName>_<Hash>.txt`。
只有本 README 与同目录的 `.gitignore` 纳入版本管理，标记文件一律不入库。

Agent 必须通过 `..\Workflow\Scripts\WorkingAgent.ps1` 操作注册表，不要手写或手删
标记文件。

## 过期判定

超过 15 分钟没有心跳的标记只判为 suspected stale，不会被自动删除或接管。
需要清理时按脚本的显式 stale 规则执行，或把判断交给用户。

## 与原生消息的关系

在支持原生任务通信的客户端里，消息可以用来提醒对方存在重叠租约，或告知某段范围
已经空出。但消息不替代本注册表：编辑前必须核对租约与实际工作区，完成时先释放
租约再通知等待方。
