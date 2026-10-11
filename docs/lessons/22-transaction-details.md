# 第二十二课：流水可读化与账单详情

## 本课定位

第二十一课已经能创建真实账单，但首页列表目前只显示金额、时间和备注，账户 UUID
与分类 UUID 对用户没有意义。本课计划建立月度流水的展示 DTO，显示账户名、分类名
并提供账单详情入口：

```text
SQLite Repositories
  -> GetMonthlyOverview
  -> MonthlyTransactionItem
  -> TransactionListModel
  -> AccountingHomePage / Detail Dialog
```

本课只增强读取和展示，不修改已有数据库结构，也不实现编辑与删除。

## 1. 本课目标

- 月度流水同时携带账户名称和分类名称。
- 保留账单 UUID、关联 UUID 与原始发生时刻，供后续操作使用。
- 首页列表以分类名称作为主要说明，备注作为补充。
- 列表显示账户名称和记账时区中的发生时间。
- 点击流水打开完整详情对话框。
- 任一关联数据查询失败时返回稳定错误，不显示半完整数据。
- 避免为每条流水分别查询账户和分类造成 N+1 查询。
- 为 Application DTO、列表模型和真实 SQLite 链路补充测试。

## 2. 当前问题

`GetMonthlyOverview` 当前只返回：

```cpp
std::vector<Transaction> transactions;
```

Transaction 必须保存 accountId 和 categoryId 才能保持领域引用，但 QML 不应把 UUID
当成用户可读文本。让 QML 自己用 UUID 再查询名称会导致：

- QML 直接接触数据访问逻辑。
- 每一行产生额外查询。
- 错误状态分散在多个 delegate 中。
- 后续编辑、删除难以获得统一详情对象。

因此关联名称应在 Application 查询流程中一次性组装。

## 3. MonthlyTransactionItem

计划新增用于读取展示的 DTO：

```cpp
struct MonthlyTransactionItem
{
    Transaction transaction;
    QString accountName;
    QString categoryName;
};
```

它不是新的领域实体，不负责验证账单规则；它只是一次月度查询的输出结构。原始
Transaction 被完整保留，避免展示 DTO 丢失后续详情需要的字段。

## 4. 避免 N+1 查询

错误做法是每读取一笔账单就执行两次 `findById()`：

```text
1 次月度流水查询 + N 次账户查询 + N 次分类查询
```

本课计划让 `GetMonthlyOverview` 一次读取全部账户和分类，建立 UUID 到名称的内存
索引，再在线性遍历流水时完成匹配：

```text
1 次月度流水查询 + 1 次账户查询 + 1 次分类查询
```

个人数据量目前不大，但尽早保持稳定的查询次数可以避免列表增长后突然变慢。

## 5. 错误语义

以下情况返回 `StorageFailure`：

- 月度流水查询失败。
- 全部账户查询失败。
- 全部分类查询失败。
- 流水引用的账户或分类无法在查询结果中找到。

数据库外键通常能防止最后一种情况，但 Application 仍不应向页面输出只有 UUID、
没有名称的半完整数据。归档账户和分类仍可以显示历史名称，归档不等于删除历史。

## 6. TransactionListModel 更新

计划新增角色：

```text
accountName
categoryName
typeText
fullOccurredAtText
```

保留已有角色：

```text
transactionId, isIncome, amountText, accountId, categoryId,
occurredAt, occurredAtText, note
```

列表和详情共享同一个模型数据来源。金额和时间仍由 C++ 按整数分、自定义记账时区
格式化，QML 不重复这些转换规则。

## 7. 首页列表调整

每一行计划显示：

```text
餐饮                         -25.50
银行卡 · 10-10 12:34
午饭
```

没有备注时不显示空白占位行。颜色继续区分收入和支出，但不能只依赖颜色表达类型，
金额前仍保留 `+` 或 `-`。

## 8. 账单详情对话框

点击列表项后展示：

- 收入或支出。
- 精确金额。
- 分类名称。
- 账户名称。
- 自定义记账时区下的完整日期和时间。
- 备注。
- 账单 UUID（可放在次要区域，主要用于诊断）。

第一版详情是只读的。后续编辑与删除会从详情入口继续扩展，但不会在本课提前加入
未验证的写操作。

## 9. 依赖变化

`GetMonthlyOverview` 将从只依赖 Transaction Repository，调整为依赖三个
Repository 接口：

```text
ITransactionRepository
IAccountRepository
ICategoryRepository
```

`AccountingModule` 已经持有三个真实 SQLite Repository，只需修改用例构造，不需要
新增全局对象或让 Presentation 访问 Infrastructure。

## 10. 测试计划

### Application

- 正确匹配账户名称和分类名称。
- 多笔流水仍只执行一次账户、分类全部查询。
- 归档账户和分类仍显示历史名称。
- 缺失关联项返回错误。
- 任意 Repository 查询失败返回错误。
- 原有月度边界、跨年和汇总结果保持不变。

### Presentation

- 新角色名称和值正确。
- 简短与完整时间都使用自定义时区。
- 模型重置后详情角色同步更新。
- 收入、支出类型文本稳定。

### QML 与真实 SQLite

- 创建账单后列表显示真实账户名与分类名。
- 点击行能打开详情。
- 重启模块后详情数据保持一致。
- QML 启动无绑定错误。

## 11. 预计文件变化

主要修改：

```text
src/features/accounting/application/GetMonthlyOverview.h/.cpp
src/features/accounting/presentation/TransactionListModel.h/.cpp
src/features/accounting/presentation/AccountingOverviewViewModel.cpp
src/features/accounting/qml/AccountingHomePage.qml
src/app/AccountingModule.cpp
相关单元测试与集成测试
```

预计不新增数据库迁移，因为账户名、分类名仍由现有表读取，不复制到 transactions
表中。

## 12. 验收清单

- [ ] 首页流水显示分类名称和账户名称。
- [ ] 点击流水可以查看完整详情。
- [ ] QML 不用 UUID 查询名称。
- [ ] 查询次数不随流水条数线性增加。
- [ ] 归档关联项的历史账单仍可读取。
- [ ] 缺失关联数据不会产生半完整页面。
- [ ] 月度统计和自定义时区行为不回退。
- [ ] 不新增不必要的数据库字段或迁移。
- [ ] 全量测试通过。
- [ ] 文档与代码导读同步更新。
