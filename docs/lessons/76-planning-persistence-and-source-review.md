# 第 76 课：规划数据持久化与原文审阅

## 本课目标

- 把规划项目、资料正文和规划版本接入 StuKit 共享 SQLite。
- 应用重启后恢复数据，同时保留内存 Repository 作为数据库不可用时的降级方案。
- 允许用户查看、修改和删除导入后的资料文本。
- 为后续模型分析保留稳定的资料快照和版本来源。

## 数据库迁移

数据库版本提升到 12，新增三张表：

- `planning_projects`：规划名称、目标、限制条件、状态和 UTC 时间；
- `planning_source_documents`：资料种类、格式、原文件名、规范化正文和 SHA-256；
- `planning_plan_revisions`：不可变版本号、Markdown、服务商、模型和引用资料 UUID。

迁移只负责创建结构，不向数据库写入示例数据。备份校验也同步认识这三张表，因此现有的
备份、恢复和安装目录数据策略仍然生效。

## Repository 替换

`PlanningWorkspace` 只依赖 Repository 接口。应用启动时，如果共享数据库可用，
`PlanningModule` 注入 SQLite 实现；否则注入内存实现并在界面显示临时存储提示。
Presentation 和 QML 不需要知道数据来自哪一种实现。

```text
PlanningViewModel -> PlanningWorkspace -> Repository interface
                                          | SQLite（正常）
                                          | Memory（降级）
```

## “原文件”和“资料正文”的区别

导入文件时，StuKit 读取受支持文件的文本副本并写入数据库，不移动、覆盖或删除用户的原
文件。资料详情弹窗展示的是数据库中的规范化正文。用户可以编辑它来清理无关内容，也可
删除这份资料；这些操作不会修改磁盘上的原始文件。

## 验证

- 数据库迁移测试确认三张表和索引存在；
- SQLite Repository 集成测试覆盖项目、正文、引用 UUID 和删除；
- 工作区测试继续覆盖内容去重和文件格式限制。

## 下一课

第 77 课建立模型服务与安全密钥端口，在不把 API Key 写入 SQLite 或设置文件的前提下，
保存 DeepSeek / OpenAI 兼容服务配置。
