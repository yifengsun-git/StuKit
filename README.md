# StuKit

StuKit 是一个面向大学生日常使用的个人工具包。项目首先开发 Windows
桌面版本，之后适配 Android。

当前规划的功能：

- 记账本：账户、分类、收支流水和月度统计。
- 课表：学期、课程、周次、节次和课程冲突检测。
- 后续模块：计时器、待办等，按实际需求逐步增加。

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

记账分析已经形成可用的独立板块：可以按用户记账时区聚合连续 2～24 个月的收入、
支出和结余，并按账户、分类进行多选过滤；所选月还会生成分类排行和账户资金流。
默认页面展示最近 12 个月的收支柱状图和结余折线图、本月汇总与上月差额；收支趋势
可以切换双折线图，分类分析可以切换横向排行和环形图。筛选面板支持 6/12/24 月、
账户与分类多选、全选、不限和恢复默认，归档项目会明确标记。图表组件消费与记账
领域无关的系列数据，未来饮食、学习等模块可以复用。分析页面现已加入 QML 集成测试，
自动验证空状态、图表切换、6/12/24 月范围、筛选摘要、ViewModel 重绑定，以及
640/960/1280 三档响应式布局；记账与分析 MVP 至此完成。

课表模块现已启动，并建立了独立的模块开发计划。领域模型、三个 Repository 契约、
内存测试实现和课程冲突检测已经完成。共享 SQLite 数据库也已升级到版本 4，新增
学期、课程和上课安排三张表，以及字段约束、外键和查询索引；版本 1～3 可以保留旧
记账数据并顺序升级，备份验证和未来版本保护已同步。全工程仍保持自动测试通过。
下一步实现三个课表 SQLite Repository。Windows Release 部署仍在课表 MVP 完成后
集中处理。

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
- [课表模块开发计划](docs/SCHEDULE_PLAN.md)
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

## 开发原则

1. QML 只负责界面展示与交互。
2. C++ 负责业务规则、用例和数据访问。
3. 核心业务不依赖具体页面或 SQLite 实现。
4. 所有数据库变更都通过版本迁移完成。
5. 每次只实现一个可验证的小目标。
