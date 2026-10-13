# StuKit

[![Windows CI and Release](https://github.com/yifengsun-git/StuKit/actions/workflows/windows.yml/badge.svg)](https://github.com/yifengsun-git/StuKit/actions/workflows/windows.yml)

StuKit 是一个面向大学生日常使用的个人工具包。项目首先开发 Windows
桌面版本，之后适配 Android。

当前规划的功能：

- 记账本：账户、分类、收支流水和月度统计。
- 课表：学期、课程、周次、节次和课程冲突检测。
- 天气：城市搜索、当前天气、短期预报和离线缓存。
- 待办：自定义分类、截止时间、优先级、提醒、归档、统计及课程绑定。
- 后续模块：计时器等，按实际需求逐步增加。

## 当前状态

项目已经完成 Qt Quick 应用入口、基础导航、主题系统和通用占位页组件，目前
正在开发记账数据层，已经实现 Money 值对象以及 Transaction、Account 和
Category 实体，并建立了 Account 与 Category 的 Repository 接口和创建用例，
同时完成 Transaction Repository 接口和 CreateTransaction 用例，以及四版 SQLite
数据库迁移和 Account、Category、Transaction 的 SQLite Repository。账单金额、
关联账户与分类、UTC 发生时间均已可以可靠持久化。月度查询与汇总用例也已完成，
月份边界使用用户自定义时区计算，不依赖设备系统时区。记账首页的 C++ ViewModel
和流水列表模型已经连接真实 SQLite 与 QML 页面，可以显示月度汇总、流水、空状态
和错误状态。自定义时区通过 QSettings 保存，默认固定为 UTC+08:00。账户与分类
首次设置也已完成：全新用户可以创建资金账户、收入分类和支出分类，期初余额以
整数分精确解析并保存；应用重启后会从真实 SQLite 恢复这些基础数据。新增账单
表单也已接通：用户可以选择收支类型、账户、匹配分类、日期时间和备注，账单保存
后月度汇总立即刷新。金额不使用浮点数，发生时间严格使用用户自定义记账时区。
流水列表现在能显示账户名、分类名和类型，点击后可查看包含完整时间与备注的
只读详情。新增账户及收支分类的入口已调整为常驻按钮，不再只能完成一次首次设置。
时区也已拥有可用的选择界面：既可选择或输入 IANA 地区时区，也可使用
UTC-14:00 到 UTC+14:00 的固定偏移，选择后会保存并同步到月度查询和账单表单。
账单详情现在可以进入编辑，修改会保留原 UUID；删除需要二次确认，两种操作
成功后都会立即刷新月度汇总和流水列表。账户与分类管理也已完成：列表会同时
显示使用中和已归档项，用户可以重命名、归档或恢复；归档项不会再进入新账单
选项，但历史账单及其可读名称仍完整保留。账户余额与总资产也已接通：每个账户
当前余额由期初余额和全部历史流水实时推导，首页总资产只汇总使用中的账户；
新增、编辑、删除账单以及归档、恢复账户都会立即刷新余额。

月度流水筛选也已完成：可按收支类型、账户、分类和备注/关联名称关键字组合过滤，
已归档关联仍可检索，筛选不会改变完整月份汇总。记账 MVP 的日常操作链路至此
完整。SQLite 数据备份与恢复也已接通：备份使用一致性快照，恢复前验证文件并自动
保留当前数据库，失败时可回退；本地数据库和备份文件均被 Git 忽略。全部历史流水
现在也能导出为 UTF-8 CSV，包含稳定的账户与分类名称、精确金额、UTC 时间和记账
时区时间，逗号、引号及多行备注会按 CSV 规则转义。首页的数据维护入口已整理为
独立卡片，统计摘要会在窄窗口下自动切换为两列。

共享分析能力已经形成可复用模块：记账本内的“数据分析”入口可以按用户记账时区聚合连续 2～24 个月的收入、
支出和结余，并按账户、分类进行多选过滤；所选月还会生成分类排行和账户资金流。
默认页面展示最近 12 个月的收支柱状图和结余折线图、本月汇总与上月差额；收支趋势
可以切换双折线图，分类分析可以切换横向排行和环形图。筛选面板支持 6/12/24 月、
账户与分类多选、全选、不限和恢复默认，归档项目会明确标记。图表组件消费与记账
领域无关的系列数据，未来饮食、学习等模块可以复用。分析页面现已加入 QML 集成测试，
自动验证空状态、图表切换、6/12/24 月范围、筛选摘要、ViewModel 重绑定，以及
640/960/1280 三档响应式布局；记账与分析 MVP 至此完成。

课表当前计划范围已完成。模块包含独立的 Domain、Application、Repository、Presentation 与
QML 分层，共享 SQLite 数据库已升级到版本 7；支持学期、课程、课程资料、多条上课
安排、任意教学周集合、冲突检测、归档和安全删除。桌面宽屏使用星期/节次网格，窄屏
阅读使用按天列表；查看与编辑模式相互分离，编辑模式也可以切换上一周、下一周和当前
周，直接观察所选教学周的实际课表。排课网格支持点击添加或修改、动态增减显示节数，
主要弹窗和编辑工具栏已适配 360 像素窄窗口。编辑器会在没有课程或没有教学周时阻止
非法创建，并明确显示当前安排的课程。Excel/WPS 课表可以另存为 UTF-8 CSV 后一键
导入，支持整学期范围或任意教学周，失败时会撤销本次已写入内容。每个节次的名称和
`HH:mm` 起止时间也可以独立修改，并会同步显示在查看和编辑网格中。SQLite、Application、
Presentation 和 QML 集成测试覆盖迁移、恢复、周次切换、自定义周次、删除、导入与
响应式布局。Windows Release 部署仍留到功能边界稳定后集中处理。

天气模块已完成第 47～52 课：领域模型、异步搜索和预报用例、Open-Meteo 网络与 JSON
解析、天气 ViewModel、应用组合根和响应式 QML 页面已经接通。用户可以搜索并保存城市，
查看当前温度、体感、湿度、降水、风况和 7 日预报；宽屏与 360 像素窄屏会使用不同布局，
页面也保留 Open-Meteo 署名。最近一次成功预报会原子写入应用数据目录中的版本化 JSON；
下次启动或断网时可以显示缓存及其更新时间，网络成功后再替换为实时数据。天气 MVP
至此完成。

待办模块已完成核心开发和验收前收口：任务领域规则、完成与归档状态流、完成率统计、
SQLite schema 10、自定义分类、课程绑定及名称快照、真实组合根和响应式清单页面已经接通。用户可以
创建、编辑、搜索、完成、重新打开、归档、恢复和确认删除任务，并按作业、考试、竞赛
分类及课程管理。到期提醒会在启动时和运行期间扫描，并通过 SQLite 交付记录避免重复
弹出；系统通知端口保留给后续 Windows 与 Android 适配。待办 MVP 已通过持久化工作流
和 QML 响应式集成验收，主清单会按逾期、今天、未来 7 天、无截止时间及状态分组。

## 技术方向

- 开发环境：Qt Creator
- UI：QML、Qt Quick Controls
- 业务逻辑：C++20
- 构建系统：CMake
- 本地数据库：SQLite、Qt SQL
- 架构：模块化单体、Clean Architecture、MVVM
- 测试：Qt Test

## 文档入口

- [项目计划](docs/PROJECT_PLAN.md)
- [CI/CD 与 Windows 发布说明](docs/CI_CD.md)
- [响应式 UI 重构计划](docs/UI_REDESIGN_PLAN.md)
- [课表模块开发计划](docs/SCHEDULE_PLAN.md)
- [天气模块开发计划](docs/WEATHER_PLAN.md)
- [架构说明](docs/ARCHITECTURE.md)
- [C++ 代码导读](docs/CPP_CODE_GUIDE.md)
- [开发协作方式](docs/DEVELOPMENT_GUIDE.md)
- [第一课：准备开发环境](docs/lessons/01-environment-setup.md)
- [第二课：创建正式工程骨架](docs/lessons/02-project-bootstrap.md)
- [第三课：建立应用导航](docs/lessons/03-app-navigation.md)
- [第四课：建立主题与通用组件](docs/lessons/04-theme-and-shared-components.md)
- [第五课：Money 值对象与自动测试](docs/lessons/05-money-and-unit-tests.md)
- [第六课：Transaction 实体与业务规则](docs/lessons/06-transaction-domain.md)
- [第七课：Account 账户实体](docs/lessons/07-account-domain.md)
- [第八课：Category 分类实体与共享领域类型](docs/lessons/08-category-domain.md)
- [第九课：Account Repository 接口与测试替身](docs/lessons/09-account-repository.md)
- [第十课：Application 层与 CreateAccount 用例](docs/lessons/10-create-account-use-case.md)
- [第十一课：SQLite 连接与数据库迁移](docs/lessons/11-sqlite-database-migrations.md)
- [第十二课：实现 SQLite Account Repository](docs/lessons/12-sqlite-account-repository.md)
- [第十三课：Category Repository 与 CreateCategory 用例](docs/lessons/13-category-repository-and-create-use-case.md)
- [第十四课：SQLite Category Repository 与迁移版本 2](docs/lessons/14-sqlite-category-repository.md)
- [第十五课：Transaction Repository 与 CreateTransaction 用例](docs/lessons/15-transaction-repository-and-create-use-case.md)
- [第十六课：SQLite Transaction Repository 与迁移版本 3](docs/lessons/16-sqlite-transaction-repository.md)
- [第十七课：按月查询流水与月度汇总](docs/lessons/17-monthly-transaction-overview.md)
- [第十八课：记账首页 ViewModel 与流水列表模型](docs/lessons/18-accounting-overview-presentation.md)
- [第十九课：应用组合根、真实数据库与 QML 接线](docs/lessons/19-accounting-composition-and-qml.md)
- [第二十课：账户与分类首次设置](docs/lessons/20-accounting-setup.md)
- [第二十一课：新增账单表单](docs/lessons/21-create-transaction-presentation.md)
- [第二十二课：流水可读化与账单详情](docs/lessons/22-transaction-details.md)
- [第二十三课：编辑与删除账单](docs/lessons/23-edit-delete-transactions.md)
- [第二十四课：账户与分类管理](docs/lessons/24-account-category-management.md)
- [第二十五课：账户余额与总资产](docs/lessons/25-account-balances.md)
- [第二十六课：月度流水筛选与搜索](docs/lessons/26-transaction-filtering.md)
- [第二十七课：SQLite 数据备份与恢复](docs/lessons/27-database-backup-and-restore.md)
- [第二十八课：流水 CSV 导出](docs/lessons/28-transaction-csv-export.md)
- [第二十九课：可自定义的记账分析查询](docs/lessons/29-accounting-analytics-query.md)
- [第三十课：分析 ViewModel 与通用图表数据模型](docs/lessons/30-accounting-analytics-presentation.md)
- [第三十一课：多图表与自定义分析交互](docs/lessons/31-accounting-analytics-charts.md)
- [第三十二课：分析页验收与记账 MVP 收尾](docs/lessons/32-accounting-analytics-acceptance.md)
- [第三十三课：课表领域模型](docs/lessons/33-schedule-domain-model.md)
- [第三十四课：课表 Repository 与冲突检测](docs/lessons/34-schedule-repositories-and-conflicts.md)
- [第三十五课：课表 SQLite 数据库迁移](docs/lessons/35-schedule-database-migration.md)
- [第三十六课：课表 SQLite Repository](docs/lessons/36-schedule-sqlite-repositories.md)
- [第三十七课：课表 Application 用例](docs/lessons/37-schedule-application-use-cases.md)
- [第三十八课：课表 Presentation 与周视图模型](docs/lessons/38-schedule-presentation.md)
- [第三十九课：学期与课程管理 UI](docs/lessons/39-schedule-term-course-ui.md)
- [第四十课：响应式周课表界面](docs/lessons/40-schedule-week-view.md)
- [第四十一课：查看模式与学期排课编辑模式](docs/lessons/41-schedule-interaction-finish.md)
- [第四十二课：课表节数、按钮反馈与课程资料](docs/lessons/42-schedule-controls-and-course-materials.md)
- [第四十三课：自定义周次与安全删除](docs/lessons/43-schedule-custom-weeks-and-deletion.md)
- [第四十四课：触摸与集成验收](docs/lessons/44-schedule-touch-and-acceptance.md)
- [第四十五课：排课防错与 CSV 批量导入](docs/lessons/45-schedule-safety-and-csv-import.md)
- [第四十六课：自定义节次名称与时间](docs/lessons/46-schedule-period-definitions.md)
- [第四十七课：天气领域模型](docs/lessons/47-weather-domain.md)
- [第四十八课：天气异步 Application 端口](docs/lessons/48-weather-application.md)
- [第四十九课：Open-Meteo 网络与 JSON 基础设施](docs/lessons/49-openmeteo-infrastructure.md)
- [第五十课：天气 Presentation 与应用组合根](docs/lessons/50-weather-presentation-and-composition.md)
- [第五十一课：响应式天气界面](docs/lessons/51-responsive-weather-ui.md)
- [第五十二课：天气离线缓存与模块验收](docs/lessons/52-weather-offline-cache-and-acceptance.md)
- [第五十三课：待办任务领域模型](docs/lessons/53-task-domain.md)
- [第五十四课：任务 Repository 与 Application 用例](docs/lessons/54-task-repository-and-use-cases.md)
- [第五十五课：任务 SQLite 持久化](docs/lessons/55-task-sqlite-persistence.md)
- [第五十六课：待办与课表课程联动](docs/lessons/56-task-course-link.md)
- [第五十七课：任务 Presentation 与应用组合根](docs/lessons/57-task-presentation-and-composition.md)
- [第五十八课：响应式任务清单界面](docs/lessons/58-responsive-task-ui.md)
- [第五十九课：可靠的任务到期提醒](docs/lessons/59-task-reminders.md)
- [第六十课：待办模块集成验收](docs/lessons/60-task-acceptance.md)
- [第六十一课：验收前产品收口](docs/lessons/61-pre-acceptance-polish.md)
- [第六十二课：Android 适配基线](docs/lessons/62-android-foundation.md)
- [人工验收清单](docs/ACCEPTANCE_CHECKLIST.md)
- [待办任务模块开发计划](docs/TASKS_PLAN.md)
- [课表 CSV 导入模板](docs/templates/schedule-import-template.csv)

## 开发原则

1. QML 只负责界面展示与交互。
2. C++ 负责业务规则、用例和数据访问。
3. 核心业务不依赖具体页面或 SQLite 实现。
4. 所有数据库变更都通过版本迁移完成。
5. 每次只实现一个可验证的小目标。

## 许可证

本项目采用 [MIT License](LICENSE)。
