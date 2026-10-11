# 第三十二课：分析页验收与记账 MVP 收尾

## 本课结果

分析页面现在拥有独立的 QML 集成测试。测试会实例化生产环境使用的
`AnalyticsHomePage` 和六个图表/筛选组件，接入真实
`AccountingAnalyticsViewModel`，验证页面契约、交互状态和响应式布局。

记账领域、SQLite、备份导出和分析查询原有测试全部继续通过。记账与分析 MVP 至此
达到当前项目计划中的完成标准，下一课开始课表模块。

## 1. 新增测试目标

```text
tests/integration/qml/
  CMakeLists.txt
  AnalyticsQmlIntegrationTest.cpp
```

CMake 注册的新测试名称为：

```text
app.analytics_qml
```

测试目标没有复制 QML 实现，而是把项目中的正式 QML 源文件编译进测试专用模块
`StuKitAnalyticsTest`。这样生产页面修改后，测试会直接针对新版本重新编译。

测试使用：

- `QApplication`：满足 Qt Charts 的样式要求。
- `QQmlEngine` 和 `QQmlComponent`：加载完整分析页面。
- Qt Test：断言属性、等待 QML 绑定更新。
- 软件渲染和 offscreen 平台：不弹出窗口即可在 CTest 中运行。

## 2. 测试数据边界

QML 测试使用正式的 `AccountingAnalyticsViewModel`、`GetAccountingAnalytics` 和
`GetAccountingSetup`，Repository 使用内存测试实现。数据包含：

- 活动账户和已归档账户。
- 活动分类和已归档分类。
- 同月收入和支出，且所选月结余为负数。
- 小于 1 元的账单。
- 跨年并位于 UTC/UTC+08:00 日期边界附近的账单。
- 完全空的数据范围。

页面测试不重新实现聚合算法。真实 SQLite 读写、重启恢复、账单变更后自动刷新和
数据备份恢复，继续由 `app.accounting_module` 及 SQLite Repository 集成测试覆盖；
所有文件都创建在 `QTemporaryDir` 或显式隔离目录中，不访问用户正式数据库。

## 3. 四组 QML 验收场景

### 空状态与响应式布局

验证：

- 空数据时柱状图、结余折线、分类排行和环形图都报告无数据。
- 640 像素宽时筛选面板默认折叠，汇总和图表使用单列。
- 960 像素宽时筛选面板展开，三个汇总卡片使用三列。
- 1280 像素宽时趋势与分类卡片使用双列。
- 账户资金流在窄窗口为单列，在桌面宽度为四列。

### 图表切换与月份范围

验证：

- 默认使用收支柱状图。
- 可以切换到收入/支出双折线图。
- 分类可以从排行切换到环形图。
- 6 个月和 24 个月切换后通用系列点数同步变化。
- 收支和负结余数据能够驱动图表的 `hasValues` 状态。

### 筛选摘要与恢复默认

验证：

- 初始账户和分类显示“不限”。
- 选择一个账户和一个分类后，摘要显示各 1 项。
- 改变月份和统计范围后，`resetFilters()` 回到 2026-10、12 个月和不限状态。

### ViewModel 重绑定

页面先连接有数据的 ViewModel，再模拟数据库恢复后由组合根创建新的空数据 ViewModel。
测试确认筛选面板和图表都改为读取新对象，旧数据不会残留在页面上。

## 4. 为测试增加的稳定页面契约

关键区域增加了 `objectName`，仅用于定位组件，不包含业务状态：

```text
analyticsFilterPanel
analyticsFilterSummary
analyticsSummaryGrid
analyticsTrendGrid
analyticsCategoryGrid
monthlyBarChart
balanceLineChart
expenseRankingBars
expenseDonutChart
```

`DonutBreakdown` 增加只读 `hasData`，测试可以验证数据状态，而不会错误依赖它是否正好
位于 `StackLayout` 当前页。`AnalyticsHomePage` 增加只读 `accountFlowColumns`，让
响应式列数成为可验证的页面契约。

## 5. 可用性修正

验收过程中统一了筛选区域的触摸高度：

- 上一月、下一月和恢复默认按钮。
- 账户与分类的全选、不限按钮。
- 账户和分类复选框。
- 6/12/24 月范围选择器。

这些控件不低于 `AppTheme.controlHeight`，当前为 44 像素。Windows 鼠标操作更稳定，
将来迁移 Android 时也不需要重新修补过小的点击区域。

## 6. CMake 测试组织

根 CMake 现在先创建正式 QML 模块，再加入 `tests/`。测试模块因此可以复用相同源文件
及资源别名。`tests/CMakeLists.txt` 新增 `integration/qml` 子目录。

QML 集成测试链接 Qt Quick、Qt Charts、Qt Widgets、Qt Test，以及记账 Application、
Domain 和 Presentation；它不链接 SQLite Infrastructure，因为页面测试使用 Repository
接口的内存实现。

## 7. 最终验收

- [x] 完整 Debug 构建通过。
- [x] 30/30 自动测试通过。
- [x] 正式应用和测试 QML 模块的 `all_qmllint` 通过。
- [x] 空数据、有数据、负结余、小额、跨年和归档关联均有自动化覆盖。
- [x] 6/12/24 月和图表切换有自动化覆盖。
- [x] 640/960/1280 三档布局有自动化覆盖。
- [x] 数据库恢复后的 ViewModel 重绑定有自动化覆盖。
- [x] 测试不读取或修改用户正式数据库。
- [x] 项目计划 M5“记账分析 MVP”标记为已完成。

## 8. 下一课

第 33 课开始课表领域模型：学期、课程、上课安排、周次模式、星期和节次。课表核心
规则仍然先在纯 C++ Domain 中完成，再进入 Repository、SQLite 和 QML。
