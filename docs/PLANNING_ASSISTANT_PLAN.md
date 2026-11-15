# StuKit 规划助手开发计划

## 1. 产品定位

规划助手不是通用聊天机器人，而是面向个人培养方案、课程、竞赛、训练营和生活目标的
结构化信息整理与规划工作区。用户主动提供资料，系统保存来源、抽取事实、执行确定性
校验，再由用户选择的大模型生成建议。任何会修改课表、待办或记账数据的动作都必须由
用户确认。

首版只开发 Windows 功能。Android 适配、云同步和移动端 Mermaid 渲染不进入本阶段，
但 Domain、Application 端口和持久化格式不得依赖 Windows 界面。

当前进度：第 74～83 课的 Windows 初版纵向闭环已完成。规划项目、资料正文和 Markdown 版本会
保存到 StuKit SQLite；用户可以查看、修改和删除资料，手动维护或导出规划，并使用
DeepSeek / OpenAI 兼容接口生成带 `[S1]` 资料引用的规划。API Key 只保存在 Windows
凭据管理器。资料会先在本地提取日期、学分、金额、时长和明确要求，并执行日期、总学分
冲突与模型引用检查。竞赛和训练营资料还会按目标相关度、时间紧迫度、预算与
每周投入、证据完整度生成可解释排名。当前模型调用为稳定优先的非流式请求，
支持 DeepSeek、OpenAI、Grok 的兼容请求与 Claude Messages 请求。Markdown 中的未完成行动
可在用户确认后发布为待办。课表和预算的只读联动留待后续增强。

## 2. 核心原则

- 本地优先：项目、资料、规划和运行记录保存在 StuKit 数据目录。
- 自带密钥：用户选择服务商并提供自己的 API Key。
- 证据优先：分析结论必须引用资料；无来源内容明确标记为推断。
- 结构化优先：AI 返回规范化数据，Markdown 是可移植的展示和编辑格式。
- 人工确认：AI 只能建议，不能直接修改其他模块或执行外部动作。
- 服务商可替换：规划业务不依赖某一家模型的请求和响应格式。
- 最小权限：导入资料不能触发命令、任意文件访问或隐藏的网络工具。

## 3. 第一版功能范围

### 必须完成

- 创建并维护一个或多个规划项目。
- 记录目标、约束、时间预算和活动预算。
- 支持粘贴文本以及导入 `.txt`、`.md`、`.csv`。
- 将资料分类为培养方案、活动机会、个人档案和其他资料。
- 保存原始文件名、导入时间、内容哈希和资料状态。
- 配置 DeepSeek 和自定义 OpenAI 兼容服务。
- 使用系统安全存储保存 API Key，不把密钥写入 SQLite、设置文件或日志。
- 分阶段完成事实抽取、规则校验、差距分析、机会评分和规划生成。
- 每条关键结论携带资料引用、置信度和必要假设。
- 保存结构化规划、Markdown 内容和历史版本。
- 导出包含 Mermaid 代码块的 Markdown 文件。
- 经用户确认后，将行动项创建为待办。

### 后续增强

- OpenAI Responses、Anthropic Messages 和 xAI Responses 适配器。
- PDF、DOCX 和扫描资料 OCR。
- SQLite FTS5 与向量混合检索。
- 规划版本对比和外部 Markdown 合并。
- 自动联网收集活动信息。
- Android 规划界面、移动端安全存储和图表渲染。
- 云同步与多设备冲突解决。

## 4. 领域模型

### `PlanningProject`

表示一项长期规划，保存 UUID、标题、目标、约束、状态、创建时间和更新时间。第一阶段
状态为草稿、进行中和已归档。持久化时间统一使用 UTC。

### `SourceDocument`

表示用户主动提供的一份资料，保存：

- 所属规划项目；
- 标题和资料种类；
- 文本、Markdown 或 CSV 格式；
- 可选的原始文件名；
- 规范化后的文本内容；
- SHA-256 内容哈希；
- UTC 导入时间。

首版单份规范化文本限制为 2 MiB。导入层必须在创建 Domain 对象前完成编码识别、文件
大小检查和格式转换。资料内容属于不可信输入，不能改变系统指令或取得工具权限。

### 后续领域对象

- `PlanningProfile`：个人能力、兴趣、可用时间和预算。
- `Opportunity`：竞赛、训练营、证书、项目或实习机会。
- `AnalysisRun`：一次分析的服务商、模型、资料快照、用量和状态。
- `PlanRevision`：结构化规划与 Markdown 的不可变历史版本。
- `EvidenceReference`：结论到资料段落的可追踪引用。
- `PlanningActionLink`：规划行动与待办、课程之间的弱引用。

## 5. 分层结构

```text
QML
  PlanningHomePage / SourceLibraryPage / AnalysisPage / PlanEditorPage
                              |
Presentation                  v
  PlanningViewModel / SourceListModel / AnalysisViewModel
                              |
Application                   v
  CreateProject / ImportSource / RunAnalysis / SavePlanRevision
  ContextBuilder / RequirementValidator / OpportunityScorer
  ILlmProvider / ISecretStore / IPlanningActionPublisher
                              |
Domain                        v
  PlanningProject / SourceDocument / PlanRevision / EvidenceReference
                              ^
Infrastructure                |
  SQLite repositories / WindowsCredentialStore / provider adapters
```

服务商适配器通过统一 `ILlmProvider` 接口工作，同时声明流式输出、JSON Schema、工具调用、
文件上传和上下文限制等能力。业务层只接收统一的结构化分析结果。

## 6. 分析流水线

```text
选择项目与资料
    -> 规范化和分段
    -> 结构化事实抽取
    -> 日期、学分、重复项和时间冲突校验
    -> 培养要求与能力差距分析
    -> 活动机会透明评分
    -> 生成结构化规划
    -> 生成 Markdown 和 Mermaid
    -> 用户审阅并产生新版本
    -> 用户确认后发布待办
```

不能用一个巨大提示词完成所有步骤。每一步保存状态并允许取消；格式修复最多自动重试
一次，无法验证的结果不得静默写入正式规划。

## 7. 数据与安全

规划数据进入 StuKit 共享 SQLite 数据库，但 API Key 只能进入平台安全存储。建议表：

```text
planning_projects
planning_profiles
planning_source_documents
planning_source_chunks
planning_opportunities
planning_analysis_runs
planning_plan_revisions
planning_action_links
planning_provider_configs   # 不含 API Key
```

发送请求前必须显示所选资料、服务商和模型。账本流水默认不参与分析。日志必须过滤授权头、
密钥、完整请求正文和个人敏感字段。

## 8. Markdown 与 Mermaid

SQLite 中的结构化规划和版本记录是事实来源，Markdown 是可读、可编辑、可导出的载体。
应用先使用 Qt Markdown 能力提供编辑与预览。Mermaid 代码写入导出文件；应用内部优先用
原生 QML 展示时间线和里程碑，避免将 Android 方案绑定到桌面 WebEngine。

## 9. 与现有模块的边界

- 课表：通过只读端口提供课程和不可用时间，不允许规划模块直接改表。
- 待办：接收用户确认后的行动草稿，保存规划来源快照。
- 记账本：只有用户主动授权时才提供预算摘要，不发送完整流水。
- 首页：后续只读取下一里程碑和风险摘要。

规划模块不能直接查询其他功能的内部数据表，跨模块协作必须使用 Application 端口。

## 10. 实施顺序

| 阶段 | 目标 | 可验收结果 |
| --- | --- | --- |
| 74 | 规划基础 | 独立分支、领域边界、项目与资料对象、可见入口 |
| 75 | 项目与资料库 | 创建项目，粘贴和导入文本、Markdown、CSV |
| 76（完成） | SQLite 持久化 | 迁移、Repository、内容去重和重启恢复 |
| 77（完成） | 模型配置 | Provider 契约、Windows 安全密钥存储、可编辑配置 |
| 78（初版完成） | DeepSeek 接入 | 非流式请求、取消、超时、错误分类、发送确认 |
| 79（完成） | 结构化抽取 | 行级证据、确定性事实抽取、规则提示、AI 输出门禁 |
| 80（完成） | 规划分析 | 确定性事实支持的透明机会评分、风险与假设 |
| 81（完成） | 规划文档 | Markdown 编辑、预览、不可变版本和导出 |
| 82（初版完成） | 模块联动 | 确认式待办草稿发布；课表约束和预算摘要留待增强 |
| 83（完成） | 多模型与验收 | OpenAI 兼容路由、Anthropic Messages、安全与 Windows 验收 |

## 11. 第一版完成标准

- 没有 API Key 时仍能创建项目、管理资料和编辑规划。
- API Key 不出现在数据库、配置、日志、导出文件和 Git 变更中。
- 每项 AI 结论能够追溯到资料，或明确显示为推断。
- 日期、学分和时间冲突由确定性规则复核。
- Markdown 规划可以编辑、导出并恢复历史版本。
- 其他模块只在用户确认后发生变更。
- Windows Debug 构建、领域测试和 QML 集成测试通过。
