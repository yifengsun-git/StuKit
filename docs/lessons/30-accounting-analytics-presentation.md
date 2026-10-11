# 第三十课：分析 ViewModel 与通用图表数据模型

## 本课结果

第 29 课的聚合结果已经通过 `AccountingAnalyticsViewModel` 接入应用。数据分析页面
现在可以显示默认分析、切换月份、选择最近 6/12/24 个月，并展示月度数据预览。

本课仍未绘制真实图表。重点是先建立稳定的 Presentation 状态和通用图表输入，让
下一课只处理可视化与交互。

## 1. 新增文件

记账专用 Presentation：

```text
src/features/accounting/presentation/
  AccountingAnalyticsViewModel.h/.cpp
  MonthlyAnalyticsListModel.h/.cpp
  CategoryAnalyticsListModel.h/.cpp
  AccountAnalyticsListModel.h/.cpp
```

通用分析 Presentation：

```text
src/features/analytics/presentation/
  CMakeLists.txt
  ChartSeriesListModel.h/.cpp
```

自动测试：

```text
tests/unit/accounting/AccountingAnalyticsPresentationTest.cpp
```

## 2. 默认分析状态

`AccountingAnalyticsViewModel` 创建时接收用户记账时区和当前月份，默认使用：

- 截止月份：当前月份。
- 时间范围：最近 12 个月。
- 账户范围：全部账户。
- 分类范围：全部分类。

ViewModel 保留初始月份。调用 `resetFilters()` 会回到初始月份、12 个月和全部账户/
分类。默认分析和自定义分析仍调用同一个 `GetAccountingAnalytics` 用例。

## 3. ViewModel 的职责

`AccountingAnalyticsViewModel` 负责：

- 调用 `GetAccountingSetup` 获得完整账户与分类筛选选项。
- 调用 `GetAccountingAnalytics` 获得聚合结果。
- 维护截止月份、月份数量、多账户和多分类选择。
- 提供上一月、下一月、恢复默认和时区同步操作。
- 格式化所选月收入、支出、结余及较上月差额。
- 把 Application DTO 分发给四个记账列表模型。
- 生成收入、支出、结余三个通用图表系列。
- 把稳定错误枚举转换为用户可以理解的中文提示。
- 失败时清理旧模型，避免继续显示失效数据。

它不执行 SQL，也不自己重新聚合账单。

## 4. 记账专用列表模型

### 月度模型

每行提供：

```text
month, monthLabel,
incomeCents, incomeText,
expenseCents, expenseText,
balanceCents, balanceText
```

`*Cents` 保留原始整数分，`*Text` 提供固定两位小数文本。图表和普通列表可以使用
同一份结果，不需要在 QML 中解析金额字符串。

### 分类模型

每行提供分类 UUID、名称、收支类型、整数分、金额文本、百分比、笔数和归档状态。
收入和支出使用两个模型，百分比分母分别是所选月总收入与总支出。

### 账户资金流模型

每行提供账户 UUID、名称、收入、支出、净流入、笔数和归档状态。金额同时具有整数分
与显示文本。

这些模型知道记账语义，因此只属于 accounting 模块。

## 5. 通用图表数据契约

`ChartSeriesListModel` 不包含账户、分类、流水或 `Money`，只暴露：

```text
ChartSeries
  seriesId
  title
  color
  points[]

ChartPoint
  label
  value
  displayValue
```

当前记账分析产生三个系列：收入、支出和结余。`value` 是交给图表坐标轴使用的
`double`，`displayValue` 是用户看到的精确两位金额；业务层与数据库仍只使用整数分。

未来饮食模块可以把热量、蛋白质或体重转换成同一契约。图表组件无需知道数据来自
记账还是饮食，这就是本课留下的复用点。

## 6. 筛选状态

ViewModel 已经支持：

- `monthCount`：2～24；当前界面提供 6、12、24 快捷项。
- `selectedAccountIds`：多账户选择。
- `selectedCategoryIds`：多分类选择。
- `accountFilterOptions`：包含 UUID、名称、归档状态和选中状态。
- `categoryFilterOptions`：额外包含收入/支出类型。
- `setAccountSelected()` 与 `setCategorySelected()`。

当前 QML 页面只接入月份和统计范围。账户、分类和图表类型的可视化选择控件会在
第 31 课完成，但底层多选逻辑及自动测试已经就绪。

## 7. 较上月差额

ViewModel 从返回的最后两个月计算：

```text
差额 = 所选月金额 - 上一个月金额
```

正数增加 `+` 前缀，负数沿用金额格式化结果，零显示 `0.00`。结余差额也进行
`qint64` 边界检查；如果两个合法结余的差额本身超出范围，则显示“超出范围”，不会
触发有符号整数溢出。

## 8. 组合根接线

`AccountingModule` 现在创建并持有：

- `GetAccountingAnalytics`。
- `AccountingAnalyticsViewModel`。

以下事件会自动刷新分析：

- 创建、编辑或删除账单。
- 创建、重命名、归档或恢复账户/分类。
- 修改用户记账时区。
- 数据库恢复后整个组合根重建。

`main.cpp` 将 ViewModel 作为 `accountingAnalytics` 根属性交给 QML；数据库恢复后也会
像其他 ViewModel 一样重新绑定，避免页面持有已经销毁的旧对象。

## 9. 页面变化

`AnalyticsHomePage.qml` 现在包含：

- 上一月、下一月。
- 最近 6/12/24 个月选择。
- 恢复默认按钮。
- 本月收入、支出、结余及较上月差额卡片。
- 连续月份数据预览。
- 当前筛选数量与记账时区说明。
- 错误和空数据状态。

月度数据预览是第 30 课的临时验证界面，第 31 课完成真实图表后会进行整理。

## 10. 自动测试与验收

新增测试验证：

- 默认月份、12 个月和无账户/分类限制。
- 月度、分类、账户及图表系列模型的角色和值。
- 金额文本、原始整数分、分类百分比和归档状态。
- 所选月汇总及上月差额。
- 多账户、多分类过滤与 6 个月范围。
- 无效筛选不污染当前状态。
- 恢复默认参数。
- Repository 失败与金额溢出会清理旧数据并产生稳定提示。
- 真实组合根能够创建和自动刷新分析 ViewModel。

验收结果：

- [x] 默认分析打开即有稳定参数。
- [x] 多账户、多分类和 2～24 个月状态可用。
- [x] QML 不接触 `Money` 或 `std::variant`。
- [x] 通用图表模型不依赖记账领域类型。
- [x] 账单、基础资料和时区变化会刷新分析。
- [x] 全部 29 个测试通过。
- [x] QML 静态检查无警告。
- [x] 隔离数据目录启动成功。

## 11. 下一课

第 31 课将在这套稳定数据上实现：

- 分组柱状图与折线图切换。
- 分类横向排行和可选环形图。
- 账户资金流展示。
- 账户、分类多选筛选控件。
- 桌面宽窗口和 Android 窄窗口响应式布局。
