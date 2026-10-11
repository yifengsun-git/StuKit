# 第三十课：分析 ViewModel 与通用图表数据模型

## 本课目标

第 29 课已经能按时间、账户和分类得到稳定的聚合结果。本课把这些 C++ 结果转换为
QML 可以直接绑定的页面状态，但暂不绘制复杂图表。完成后，第 31 课只需专注组件和
交互，不再触碰统计规则。

## 1. 默认状态

`AccountingAnalyticsViewModel` 首次加载时使用：

- 用户记账时区中的当前月份。
- 最近 12 个月。
- 全部账户和全部分类。
- 收入、支出、结余三种基础指标。

用户修改参数后调用同一个 `GetAccountingAnalytics` 用例。恢复默认值也只是重置这些
参数，不走另一条代码路径。

## 2. 计划新增的类型

```text
src/features/accounting/presentation/
  AccountingAnalyticsViewModel.h/.cpp
  MonthlyAnalyticsListModel.h/.cpp
  CategoryAnalyticsListModel.h/.cpp
  AccountAnalyticsListModel.h/.cpp
```

三个模型分别承担：

- 月度模型：月份标签、收入、支出、结余的整数分和显示文本。
- 分类模型：分类名称、金额、占比、笔数、收入/支出类型和归档状态。
- 账户模型：账户名称、收入、支出、净流入、笔数和归档状态。

ViewModel 负责调用用例、替换模型内容、计算上月差额、维护筛选状态和转换错误提示。
列表模型不读取 Repository，QML 不重新计算金额。

## 3. 可自定义参数

ViewModel 将暴露：

- `selectedYear`、`selectedMonth` 和可读月份标签。
- `monthCount`，接受 2～24，UI 提供 6、12、24 快捷项。
- 可选账户 UUID 集合。
- 可选分类 UUID 集合。
- 可用于筛选的账户和分类选项模型。
- `resetFilters()`、`reload()`、上一月和下一月操作。
- `loading`、`hasData`、`errorMessage`。

第一版不持久化用户自定义筛选。应用重启后恢复默认分析，以免旧筛选让用户误以为数据
丢失。后续如果确有需求，再单独设计保存策略。

## 4. 通用图表数据契约

具体业务模型不会直接成为可复用图表组件的输入。Presentation 会额外整理出轻量的
展示契约：

```text
ChartSeries
  seriesId
  title
  color
  values[]

ChartPoint
  label
  value
  displayValue
```

记账可以输出“收入”“支出”“结余”系列；未来饮食模块可输出“热量”“蛋白质”系列。
图表组件只认识这个契约，不包含 `TransactionType`、账户或营养业务规则。

本课会优先使用 Qt 友好的 `QAbstractListModel` 和只读属性，而不是让 QML 解析随意结构的
JavaScript 对象。若 Qt Charts 的系列更新 API 需要适配，则将适配放在通用分析组件边界。

## 5. 错误映射

ViewModel 将稳定映射用例错误：

- `InvalidInput`：分析条件无效，请恢复默认条件。
- `StorageFailure`：暂时无法读取记账数据。
- `ArithmeticOverflow`：金额过大，无法安全完成统计。

加载失败时清空旧图表数据，避免页面继续展示已经失效的结果。

## 6. 组合根接线

`AccountingModule` 将创建 `GetAccountingAnalytics` 和
`AccountingAnalyticsViewModel`，并把 ViewModel 作为根属性提供给分析页面。以下变化会
触发重新加载：

- 新增、编辑或删除流水。
- 账户或分类名称、归档状态变化。
- 用户更改记账时区。
- 数据库恢复后组合根重建。

分析页面不会直接持有 Repository 或数据库对象。

## 7. 自动测试计划

- 默认值为当前月、12 个月和无账户/分类限制。
- 月度、分类、账户模型角色完整且顺序稳定。
- 金额文本保持两位小数，原始整数分可供图表使用。
- 上月差额和变化方向正确，包括零值与负结余。
- 账户/分类筛选和重置会重新加载。
- 2 和 24 个月有效，边界外输入不会污染当前状态。
- 三类用例错误映射为稳定提示，并清空旧数据。
- 归档关联保留名称及状态。
- 记账数据变化和时区变化能刷新分析。
- 原有 28 个测试继续通过。

## 8. 验收标准

- QML 无需接触 `Money` 或 `std::variant`。
- 默认分析打开即有合理参数。
- 自定义时间、账户和分类筛选有单一状态来源。
- 通用图表输入不依赖记账领域类型。
- ViewModel 不执行 SQL，模型不重新统计业务金额。
- 自动测试、QML 静态检查和启动检查全部通过。

完成本课后，第 31 课将接入分组柱状图、折线图、横向排行和可选环形图，并完成宽窄
窗口布局与交互。
