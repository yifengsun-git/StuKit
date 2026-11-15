# 第 74 课：规划助手基础与 Android 工作隔离

## 本课目标

- 把未完成的 Android 适配与 Windows 新功能隔离。
- 明确规划助手不是通用聊天机器人。
- 建立规划项目与资料文档的领域规则。
- 在 Windows 主导航和首页加入规划助手入口。
- 为后续资料库、Provider 和规划版本保留清晰分层。

## 分支与工作区

Android 适配修改保存在 Git stash：

```text
stash@{0}: wip(android): preserve adaptation before planning module
```

规划助手从 `v0.3.0` 对应的干净 `main` 基线创建分支：

```text
codex/planning-assistant
```

在规划助手开发期间不要应用该 stash，也不要把 Android WIP 混入本分支提交。恢复 Android
工作前应先确认当前规划代码已经提交或另行保存。

## 新增领域对象

### `PlanningProject`

规划项目需要稳定 UUID、非空标题和目标、可选约束、状态以及 UTC 创建和更新时间。对象
在创建时统一清理文本首尾空白，并拒绝无效时间和超长字段。

### `SourceDocument`

资料文档属于某个规划项目。第一版接受纯文本、Markdown 和 CSV 的规范化文本，记录资料
种类、原始文件名、UTC 导入时间和 SHA-256 内容哈希。行尾统一为 `LF`，方便跨文件来源
比较和去重。

文件读取、编码判断和 CSV 解析不属于 Domain；下一课通过 Application 端口和
Infrastructure 适配器接入。

## 为什么现在不接 API

如果先接 API，再补资料身份、内容哈希、引用和版本，会让聊天响应成为无法验证的临时
文本。本课先确定输入和规划项目的身份，下一阶段才能可靠记录“哪次分析使用了哪些资料”。

## 当前可验收内容

- 主导航和首页能够打开“规划助手”。
- 页面明确显示资料、分析、规划三个阶段及首版安全边界。
- 领域测试覆盖文本规范化、UTC、标识、时间顺序、内容哈希和无效资料。
- 没有引入模型 SDK、API Key、网络请求或数据库迁移。

## 下一课

第 75 课实现项目与本地资料库的 Application 用例和 Presentation：创建规划项目、粘贴
文本，并从 `.txt`、`.md`、`.csv` 导入资料。此阶段仍不调用真实模型。
