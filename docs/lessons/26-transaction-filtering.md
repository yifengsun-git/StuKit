# 第二十六课：月度流水筛选与搜索

## 本课定位

第二十五课已经完成账户余额和总资产。随着流水增加，只按月份浏览会越来越难找到
目标记录。本课将为当前月份列表增加组合筛选，但不修改或复制账单数据。

## 1. 本课目标

- 按全部、收入、支出筛选。
- 按活动或已归档账户筛选。
- 按收入/支出分类筛选。
- 按备注、账户名或分类名进行关键字搜索。
- 多个条件同时生效，并可一键清空。
- 筛选后显示结果数量和无匹配状态。
- 月收入、月支出和月结余继续表示完整月份，不随列表筛选变化。
- 月份、基础资料或流水变化后保留合理筛选状态并刷新结果。

## 2. 为什么先在 Presentation 筛选

月度流水已经由 `GetMonthlyOverview` 一次读取，个人账本单月数据量通常较小。
第一版可在 Presentation 使用代理模型筛选现有列表：

```text
TransactionListModel
  -> TransactionFilterProxyModel
  -> QML ListView
```

这样不需要扩大 SQL 接口，也不会让 QML 自己遍历和复制模型。若以后单月数据达到
需要分页的规模，再把查询条件下推到 Application 和 Repository。

## 3. 筛选状态

计划提供：

```text
typeFilter       all / income / expense
accountIdFilter  empty means all
categoryIdFilter empty means all
searchText       trimmed, case-insensitive
resultCount
hasActiveFilters
```

账户和分类下拉框仍显示已归档项，因为历史流水可能引用它们。分类选项会随收支类型
筛选联动，但不会修改账单。

## 4. 界面计划

- 流水列表上方增加可展开的筛选区域。
- 使用清晰文字标签，不只依靠图标或颜色。
- 条件生效时显示“清除筛选”。
- 月份有流水但没有匹配项时显示“没有符合条件的账单”。
- 点击筛选结果仍能进入原账单详情、编辑和删除流程。

## 5. 测试计划

- 单独验证收支、账户、分类和关键字筛选。
- 验证多个条件取交集。
- 验证搜索忽略大小写并清理首尾空白。
- 验证已归档账户与分类的历史流水仍可筛选。
- 验证源模型重置后代理模型自动更新。
- 验证清空条件恢复完整月度列表。
- 验证 QML 静态检查和实际启动。

## 6. 预计文件变化

```text
src/features/accounting/presentation/TransactionFilterProxyModel.h/.cpp
src/features/accounting/presentation/AccountingOverviewViewModel.h/.cpp
src/features/accounting/qml/AccountingHomePage.qml
相关 Presentation 测试与文档
```

预计不修改 Domain、SQLite 表结构和数据库迁移。

## 7. 验收清单

- [ ] 收支类型筛选正确。
- [ ] 账户和分类筛选正确。
- [ ] 关键字搜索正确。
- [ ] 多条件组合与一键清空正确。
- [ ] 归档关联的历史流水仍可筛选。
- [ ] 筛选不改变完整月份汇总。
- [ ] 详情、编辑和删除入口继续工作。
- [ ] 全量测试、QML lint 和启动检查通过。
- [ ] 文档与代码导读同步更新。
