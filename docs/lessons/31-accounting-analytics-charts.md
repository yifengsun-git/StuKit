# 第三十一课：多图表与自定义分析交互

## 本课结果

记账分析不再是数据预览页。现在打开“数据分析”即可看到默认图表，也可以切换图表
形式，按截止月份、统计范围、账户和分类重新分析。数据库结构与聚合用例没有改变，
本课主要完成 QML 可视化边界和筛选交互。

## 1. 新增 QML 组件

```text
src/features/analytics/qml/components/
  AnalyticsCard.qml
  AnalyticsFilterPanel.qml
  BarSeriesChart.qml
  LineSeriesChart.qml
  RankingBars.qml
  DonutBreakdown.qml
```

- `AnalyticsCard` 统一分析卡片的标题、说明、背景和间距。
- `AnalyticsFilterPanel` 提供月份、范围、账户和分类筛选。
- `BarSeriesChart` 绘制收入与支出的分组柱状图。
- `LineSeriesChart` 可绘制单折线或双折线，目前用于结余及收入/支出趋势。
- `RankingBars` 用于分类金额、占比和笔数排行。
- `DonutBreakdown` 用环形图显示分类构成，并提供文字图例。

这些组件只接收模型、系列 ID、标签、数值、颜色和显示文本，不查询数据库，也不调用
记账 Application 用例。

## 2. 默认分析页面

首次进入页面不要求配置，默认显示：

- 最近 12 个月收入与支出的分组柱状图。
- 最近 12 个月结余折线图。
- 所选月收入、支出、结余及较上月差额。
- 所选月支出和收入分类横向排行。
- 所选月各账户收入、支出、净额及账单笔数。

没有数据时显示明确的空状态，不显示没有意义的空坐标轴。

## 3. 图表切换

月度收支支持：

```text
分组柱状图 <-> 双折线图
```

收入和支出分类分别支持：

```text
横向排行 <-> 环形图
```

结余保留折线图，因为它需要清楚表现正负变化。结余小于零时，纵轴会自动扩展到
零线以下。

## 4. 自定义筛选面板

筛选面板已经接通第 30 课的 ViewModel：

- 上一月和下一月。
- 最近 6、12、24 个月。
- 账户多选，归档账户带有明确标记。
- 分类多选，同时显示收入/支出类型和归档标记。
- 账户与分类分别提供“全选”和“不限”。
- “恢复默认”回到初始月份、12 个月和无额外限制。

这里的“不限”表示不按该维度过滤。因此没有勾选任何账户时统计全部账户，没有勾选
任何分类时统计全部分类，而不是返回空数据。

为避免点击“全选”时连续查询多次，ViewModel 新增四个批量操作：

```text
selectAllAccounts()
clearAccountSelection()
selectAllCategories()
clearCategorySelection()
```

每个批量操作最多触发一次重新聚合。

## 5. 通用图表数据快照

`ChartSeriesListModel` 增加只读 `items` 属性，把所有系列作为 `QVariantList` 快照交给
QML。每项仍使用第 30 课定义的通用契约：

```text
seriesId, title, color, points[]
point: label, value, displayValue
```

QML 用 `seriesId` 查找收入、支出或结余系列。组件不认识 `Money`、账户、分类或
`TransactionType`，以后其他功能只要产生相同契约即可复用柱状图和折线图。

## 6. Qt Charts 的依赖边界

根 CMake 现在查找并链接 `Qt6::Charts`。Qt Charts 只进入最终 `StuKit` 可执行程序和
三个 QML 图表组件，以下目标均没有新增 Qt Charts 依赖：

- Domain。
- Application。
- Infrastructure。
- `StuKitAnalyticsPresentation`。
- `StuKitAccountingPresentation`。

Qt Charts 的 QML 图表需要 Qt Widgets 提供样式服务，因此入口由 `QGuiApplication`
调整为 `QApplication`，最终程序链接 `Qt6::Widgets`。这只是应用外壳的运行时要求，
业务和数据层仍不依赖 Widgets。

Qt 6.10.1 自带的 Qt Charts `plugins.qmltypes` 存在版本元数据警告，组件只在
`import QtCharts` 一行附近抑制该第三方导入警告；项目自身的 QML 警告仍会正常报告。

## 7. 响应式布局

- 宽窗口中趋势卡片、分类卡片使用双列布局。
- 普通桌面宽度自动改为单列图表，避免坐标轴拥挤。
- 汇总卡片在宽度不足时从三列切换为单列。
- 窄窗口默认折叠筛选面板，用户可以随时展开。
- 整个页面使用纵向滚动，不靠压缩卡片隐藏操作。
- 24 个月时横轴标签自动旋转并减少折线图下方的可见标签。

这套布局仍需要在第 32 课用实际窗口尺寸做系统验收。

## 8. 测试与问题修复

`AccountingAnalyticsPresentationTest` 增加验证：

- 通用系列快照包含收入、支出和结余。
- 账户全选与不限。
- 分类全选与不限。

本课构建时还排除了两个环境/运行时问题：

1. 新建命令行构建目录曾报告 MSVC 程序数据库管理器不匹配；继续使用 Qt Creator
   已配置的 Kit 构建目录可以稳定构建，这不是项目源码错误。
2. 首次启动图表时 Qt Charts 因缺少 `QApplication` 样式服务触发断言；入口改用
   `QApplication` 后隔离目录启动正常。

最终验收：

- [x] Qt Charts 只位于可视化/应用外壳边界。
- [x] 默认柱状图、结余折线图和分类排行已接入。
- [x] 收支折线及分类环形图可以切换。
- [x] 账户与分类多选、全选、不限和恢复默认已接入。
- [x] 归档账户与分类有明确标记。
- [x] 空数据有稳定提示。
- [x] 完整构建通过。
- [x] 全部 29 个测试通过。
- [x] `all_qmllint` 无项目警告。
- [x] 隔离数据目录启动检查通过。

## 9. 下一课

第 32 课将为分析页补充 QML 集成测试和真实数据验收，重点检查窄窗口、图表切换、
筛选重置、归档数据和数据库恢复后的重新绑定。记账 MVP 验收完成后，第 33 课开始
课表领域模型。
