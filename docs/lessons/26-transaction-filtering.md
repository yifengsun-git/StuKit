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

## 4. 界面实现

- 流水列表上方增加筛选区域。
- 使用清晰文字标签，不只依靠图标或颜色。
- 条件生效时显示“清除筛选”。
- 月份有流水但没有匹配项时显示“没有符合条件的账单”。
- 点击筛选结果仍能进入原账单详情、编辑和删除流程。

## 5. 测试范围

- 单独验证收支、账户、分类和关键字筛选。
- 验证多个条件取交集。
- 验证搜索忽略大小写并清理首尾空白。
- 验证已归档账户与分类的历史流水仍可筛选。
- 验证源模型重置后代理模型自动更新。
- 验证清空条件恢复完整月度列表。
- 验证 QML 静态检查和实际启动。

## 6. 文件变化

```text
src/features/accounting/presentation/TransactionFilterProxyModel.h/.cpp
src/features/accounting/presentation/AccountingOverviewViewModel.h/.cpp
src/features/accounting/qml/AccountingHomePage.qml
相关 Presentation 测试与文档
```

预计不修改 Domain、SQLite 表结构和数据库迁移。

## 7. 验收清单

- [x] 收支类型筛选正确。
- [x] 账户和分类筛选正确。
- [x] 关键字搜索正确。
- [x] 多条件组合与一键清空正确。
- [x] 归档关联的历史流水仍可筛选。
- [x] 筛选不改变完整月份汇总。
- [x] 详情、编辑和删除入口继续工作。
- [x] 全量测试、QML lint 和启动检查通过。
- [x] 文档与代码导读同步更新。

## 8. 实现说明

`TransactionFilterProxyModel` 继承 `QSortFilterProxyModel`，它不保存第二份流水，
而是逐行判断源模型中的账单是否满足全部条件。类型、账户、分类和关键字条件之间
是“并且”关系；关键字对备注、账户名和分类名执行不区分大小写的包含匹配。

`AccountingOverviewViewModel` 仍持有完整的 `TransactionListModel`，但把代理模型
暴露给 QML。月收入、月支出和月结余来自 `GetMonthlyOverview` 的完整结果，因此
改变筛选条件只会改变列表行数，不会改变月度汇总。

账户与分类选项从当前月份的完整流水生成，而不是从“未归档基础资料”生成。因此，
历史账单引用的已归档账户与分类仍能出现在筛选列表中。切换收入/支出类型时，分类
选项同步收窄；如果原分类与新类型不兼容，会自动清除该分类条件。

## 9. 实现与验证结果

- 已支持全部、收入和支出三种类型范围。
- 已支持账户、分类和关键字筛选，多个条件取交集。
- 已显示匹配数量、无匹配状态和“清除筛选”按钮。
- 源模型重新加载后，代理模型和下拉选项同步刷新。
- 已归档关联仍保留可读名称并可用于筛选。
- 原有流水详情、编辑和删除入口继续使用代理模型提供的角色。

验证结果：

```text
MSVC 2022 Release build: passed
CTest: 24/24 passed
QML lint: passed with no warnings
Windows QML startup smoke test: passed (no stderr output)
```

排查过程中曾出现一次仅限 Release 集成测试退出阶段的异常。使用带符号 Release
和 Debug 复测后无法稳定复现；清理旧的混合增量产物并按标准 Release 参数完整重建
后，目标测试和全量测试均通过。此问题判断为构建缓存混用，而非筛选功能的稳定缺陷。

## 10. 后续方向

第二十七课进入可靠性阶段，实现 SQLite 数据库的安全备份与恢复。备份和恢复能力
会放在独立服务中，界面只负责选择文件和确认操作，为后续 Android 文件选择器适配
保留边界。
