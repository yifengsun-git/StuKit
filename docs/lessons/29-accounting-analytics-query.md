# 第二十九课：记账分析查询与聚合模型

## 本课定位

记账本已经能够可靠记录、修改、筛选、备份和导出数据，但“有数据”还不等于“能看懂
数据”。在开始课表模块前，先补齐记账分析 MVP。本课只建立可测试的数据查询和聚合
能力；下一课再创建 ViewModel 和图表页面。

当前 MSVC Qt 6.10.1 Kit 已安装 `Qt6Charts`。图表 UI 可以直接使用 Qt Charts，但
分析数据不会依赖 Qt Charts，这样将来更换图表组件或迁移 Android 时不用重写统计
规则。

## 1. 分析 MVP 的范围

第一版提供三类信息：

1. 近 12 个月收入、支出和结余趋势。
2. 所选月份支出分类排行与收入分类排行。
3. 所选月份与上一个月的收入、支出和结余对比。

暂不实现预算、资产收益率、债务分析、消费预测、AI 建议和自定义报表。这些属于
后续增强，不阻塞个人账本的基础分析价值。

## 2. 页面最终效果

```text
记账分析
  [所选月份] [近 6 月 / 近 12 月]

  本月支出      较上月 +120.00
  本月收入      较上月 -300.00
  本月结余      较上月 -420.00

  收支趋势
  ┌──────────────────────────────┐
  │ 每月收入与支出的分组柱状图    │
  └──────────────────────────────┘

  支出分类排行
  餐饮       680.00   42.50%
  交通       320.00   20.00%
  ...
```

分类排行第一版使用横向进度条和文字，不使用饼图。分类较多时，排行比大量扇区更容易
阅读，也更适合以后缩窄到 Android 手机宽度。

## 3. 分析口径

### 月份范围

- 默认展示以“所选月份”为终点的最近 12 个自然月。
- 后续 UI 可以切换最近 6 个月或 12 个月。
- 没有流水的月份也要返回零值，保证横轴连续。
- 月份边界由用户自定义记账时区计算，再转换为 UTC 查询。

例如所选月份为 2026-10，近 12 个月范围是 2025-11 至 2026-10。

### 收入、支出和结余

```text
月结余 = 月收入 - 月支出
对比差额 = 所选月金额 - 上月金额
```

业务聚合始终使用 `Money` 的整数分，不使用浮点数。同比例图表绘制时可以在
Presentation/QML 中临时转为 `double`，但数据库、Application 结果和金额文本仍以
整数分为准。

### 分类排行

- 收入和支出分别统计，不混在一个百分比中。
- 金额按降序排列，相同金额再按分类名称和 UUID 稳定排序。
- 已归档分类仍保留历史名称并参与统计。
- 分类占比的分母是所选月份同类型的总金额。
- 百分比只用于显示，原始 `Money` 金额仍是统计依据。

## 4. Application 输出模型

计划新增：

```cpp
struct GetAccountingAnalyticsRequest
{
    QDate selectedMonth;
    int monthCount{12};
    QTimeZone timeZone;
};

struct MonthlyAnalyticsPoint
{
    QDate month;
    Money income;
    Money expense;
    Money balance;
};

struct CategoryAnalyticsItem
{
    QUuid categoryId;
    QString categoryName;
    TransactionType type;
    Money amount;
    int transactionCount{0};
};

struct AccountingAnalytics
{
    std::vector<MonthlyAnalyticsPoint> months;
    std::vector<CategoryAnalyticsItem> incomeCategories;
    std::vector<CategoryAnalyticsItem> expenseCategories;
};
```

上月对比不需要额外保存重复字段：Presentation 从最后两个月的点计算差额。百分比也
不放在 Application DTO 中，避免把图表显示格式混进用例层。

## 5. 查询流程

```text
GetAccountingAnalytics::execute(request)
  -> 校验月份、monthCount 和 QTimeZone
  -> 计算最早月份起点和所选月份下月起点
  -> 转换为 UTC
  -> ITransactionRepository::findOccurredBetween（只查询一次）
  -> ICategoryRepository::findAll（只查询一次）
  -> 建立分类 UUID 到名称/类型的索引
  -> 初始化连续月份零值
  -> 按用户时区把每笔流水归入月份
  -> 聚合月度值与所选月分类值
  -> 稳定排序并返回
```

不能循环 12 次分别查询数据库。一次时间范围查询加一次分类查询能够避免重复 SQL，
也让 SQLite 实现和测试替身保持简单。

## 6. 错误与边界规则

结果使用 `std::variant<AccountingAnalytics, GetAccountingAnalyticsError>`。

错误包括：

- `InvalidInput`：月份无效、月份数量不在 2～24 之间或时区无效。
- `StorageFailure`：流水或分类查询失败、流水引用缺失分类。
- `Overflow`：收入、支出或分类金额相加超出 `qint64`。

关键边界：

- 至少查询 2 个月，否则无法进行上月对比。
- 最多允许 24 个月，避免 UI 一次加载过多无意义点位。
- 发生时间必须先转入用户记账时区，再判断所属月份。
- 夏令时地区也使用 `QDate::startOfDay(QTimeZone)` 计算边界。
- 成功但没有流水时返回连续月份的零值，不是错误。

## 7. 文件结构

```text
src/features/accounting/application/
  GetAccountingAnalytics.h
  GetAccountingAnalytics.cpp

tests/unit/accounting/
  GetAccountingAnalyticsTest.cpp
```

本课不修改 SQLite 表，因为分析完全来自现有 Transaction 和 Category 数据。

## 8. 自动测试计划

- 空账本返回 12 个连续零值月份和空分类排行。
- 收入、支出和结余按月正确聚合。
- 所选月份跨年时范围正确，例如 2026-01 能回溯到 2025 年。
- 用户时区月初边界正确，不依赖电脑系统时区。
- 已归档分类保留原名称。
- 分类金额与笔数聚合正确并稳定排序。
- 只调用一次流水范围查询和一次分类全部查询。
- 无效月份、月份数量和时区返回 `InvalidInput`。
- Repository 失败和缺失分类返回 `StorageFailure`。
- 整数累加溢出返回 `Overflow`，不返回部分结果。
- 原有 27 个测试继续通过。

## 9. 本课不做什么

- 不加入 Qt Charts 依赖。
- 不创建 QML 图表页面。
- 不修改导航结构。
- 不保存冗余统计表或缓存。
- 不增加数据库迁移。

先让聚合规则脱离界面通过测试，下一课再把稳定结果转换为图表模型。

## 10. 验收清单

- [ ] 可以获得连续 6～24 个月的月度趋势点。
- [ ] 所选月的收入、支出分类排行正确。
- [ ] 所有金额聚合保持整数分。
- [ ] 用户自定义时区决定月份边界。
- [ ] 已归档分类仍保持可读名称。
- [ ] 查询次数固定，不随月份数量增加。
- [ ] 错误、缺失关联和溢出有稳定结果。
- [ ] 新增测试与原有测试全部通过。
- [ ] C++ 代码导读同步更新。

## 11. 后续课程路线

```text
第 29 课  分析查询与月度/分类聚合
第 30 课  AnalyticsViewModel 与图表数据模型
第 31 课  Qt Charts 分析页面、响应式布局与交互
第 32 课  分析模块集成测试、空状态和记账 MVP 收尾
第 33 课  课表星期、节次和教学周值对象
第 34 课  AcademicTerm 学期实体与业务规则
第 35 课  Course 与 CourseMeeting 实体
第 36 课  课程冲突检测用例
第 37 课  课表 Repository 与 SQLite 持久化
第 38 课  课表 ViewModel、周视图和课程编辑
第 39 课  课表 MVP 收尾与数据可靠性
第 40 课  Windows Release 构建和文件夹部署
```

课表开发不会取消，只是顺延到分析模块验收完成之后。Windows 部署继续放在功能
边界稳定之后集中处理。
