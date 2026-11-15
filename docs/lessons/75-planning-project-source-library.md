# 第 75 课：规划项目与本地资料库

## 本课目标

- 创建规划项目，记录目标和限制条件。
- 粘贴纯文本、Markdown 或 CSV 资料。
- 导入 UTF-8 编码的 `.txt`、`.md`、`.markdown` 和 `.csv` 文件。
- 按培养方案、竞赛或训练营、个人档案和其他资料分类。
- 在同一规划内使用内容哈希阻止重复资料。
- 保持文件读取、业务用例、Presentation 和 QML 分离。

## Application 工作区

`PlanningWorkspace` 是当前阶段的应用服务，协调项目 Repository、资料 Repository 和文件
读取端口。它负责创建 UUID、使用 UTC 时间、验证项目是否存在，并将文件内容转换成
Domain 对象。它不依赖 QML，也不直接使用 `QFile`。

```text
PlanningViewModel
        |
PlanningWorkspace
   |          |          |
ProjectRepo SourceRepo ISourceFileReader
```

文件读取通过 `ISourceFileReader` 端口完成。Windows 首版实现 `TextSourceFileReader`，只
接受受控扩展名和 UTF-8，读取前检查 2 MiB 大小限制。导入文件不会被复制到安装目录，
而是读取并规范化为资料内容；原始文件名仅作为来源快照。

## 为什么暂时使用内存 Repository

本课重点是确认用例、交互和数据边界。`InMemoryPlanningProjectRepository` 与
`InMemorySourceDocumentRepository` 让完整界面可以运行，同时避免在 schema 未确定时
修改共享数据库。

页面会明确提示当前数据只保留在运行会话中。第 76 课实现 SQLite migration 和
Repository 后，`PlanningWorkspace` 与 QML 不需要改变，只在组合根替换适配器。这正是
Repository 端口带来的价值。

## 重复资料规则

`SourceDocument` 对规范化内容计算 SHA-256。资料 Repository 在同一个规划项目内发现
相同哈希时拒绝保存，避免用户重复粘贴或导入同一份通知。不同规划允许引用相同内容，
因为它们拥有不同的分析上下文。

哈希用于去重和变更识别，不用于加密，也不能替代 API Key 的安全存储。

## Presentation

`PlanningViewModel` 向 QML 提供：

- 项目和资料列表；
- 当前选中的规划；
- 资料种类和格式选项；
- 创建项目、选择项目、粘贴资料和导入文件操作；
- 用户可读的成功与错误消息。

QML 不解释领域枚举，也不直接读取文件。资料卡片显示种类、格式、导入时间、摘要和短
哈希，方便用户在调用 AI 之前检查上下文。

## 当前界面流程

1. 打开“规划助手”。
2. 创建规划，填写名称、主要目标和可选约束。
3. 在当前规划下选择“粘贴资料”或“导入文件”。
4. 为资料选择种类；粘贴时还可以指定文本格式。
5. 查看资料摘要和来源信息。
6. 切换规划时，资料列表只显示当前规划内容。

## 自动化测试

测试覆盖：

- 创建项目和添加资料；
- 规划不存在时拒绝资料；
- 相同内容去重；
- UTF-8 Markdown 文件导入；
- 不支持的扩展名；
- ViewModel 状态和列表更新；
- 首页到规划助手的导航信号。

## 下一课

第 76 课为共享数据库增加规划迁移，并实现 SQLite 项目与资料 Repository。完成后项目和
资料将在应用重启后保留，同时仍不接入真实模型 API。
