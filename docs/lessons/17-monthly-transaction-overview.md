# 第十七课：按月查询流水与月度汇总

## 本课定位

第十六课已经能把账单可靠保存到 SQLite。本课开始实现记账首页真正需要的读取能力：
按照用户所在时区查询某个月的流水，并计算收入、支出和结余。

```text
Presentation（以后选择月份）
  -> GetMonthlyOverview
  -> ITransactionRepository::findOccurredBetween()
  -> SqliteTransactionRepository
  -> transactions 表
```

本课先完成 Domain、Application、Infrastructure 和自动测试，不制作 QML 页面。

本项目不直接使用设备系统时区。`GetMonthlyOverviewRequest` 显式接收用户选择的
`QTimeZone`，既支持 `Asia/Shanghai` 这类时区，也支持 UTC+08:00 这类固定偏移。
后续设置页面负责保存用户的选择。

## 1. 学习目标

- 理解为什么“查询十月份”必须转换成明确的起止时刻。
- 使用左闭右开区间表达时间范围。
- 扩展 Repository 接口，而不是让 Application 直接写 SQL。
- 在 Application 层计算月收入、月支出和月结余。
- 让内存 Repository 与 SQLite Repository 遵守同一查询契约。
- 覆盖月份边界、时区、空月份和数据库查询失败等场景。

## 2. 月份不是一个 UTC 日期范围

用户看到的是本地月份。例如上海时区的 2026 年 10 月应表示：

```text
本地开始：2026-10-01 00:00:00 Asia/Shanghai
本地结束：2026-11-01 00:00:00 Asia/Shanghai

转换 UTC：
2026-09-30T16:00:00.000Z
到
2026-10-31T16:00:00.000Z
```

如果直接按 UTC 的 10 月 1 日到 11 月 1 日查询，本地凌晨发生的账单就可能被放进
错误月份。Windows 和 Android 都应使用同一套规则。

## 3. 使用左闭右开区间

时间范围采用：

```text
[startInclusive, endExclusive)
```

含义是：

- 包含月份第一天 00:00:00。
- 不包含下个月第一天 00:00:00。

对应查询条件：

```sql
WHERE occurred_at >= :startInclusive
  AND occurred_at < :endExclusive
ORDER BY occurred_at DESC, id
```

这种方式无需猜测一个月最后一天是 28、29、30 还是 31 日，也无需制造
`23:59:59.999` 这样的脆弱边界。

## 4. 扩展 Repository 契约

在 `ITransactionRepository` 增加明确的时间范围查询：

```cpp
virtual std::optional<std::vector<Transaction>> findOccurredBetween(
    const QDateTime &startInclusive,
    const QDateTime &endExclusive) const = 0;
```

返回值约定：

- 有值且 vector 为空：查询成功，但这个月没有账单。
- 有值且 vector 非空：查询成功并返回账单。
- `std::nullopt`：存储查询失败或读到了无效数据库行。

这里不能只返回 vector，否则“没有数据”和“数据库出错”都会表现为空列表，
Application 无法给 Presentation 正确结果。

需要同步修改：

- `InMemoryTransactionRepository`
- `SqliteTransactionRepository`
- Repository 契约测试

已有 `findAll()` 暂时保留，避免把本课变成无关的接口清理工作。

## 5. 月度用例数据结构

新增：

```text
src/features/accounting/application/GetMonthlyOverview.h
src/features/accounting/application/GetMonthlyOverview.cpp
tests/unit/accounting/GetMonthlyOverviewTest.cpp
```

建议的请求：

```cpp
struct GetMonthlyOverviewRequest
{
    int year;
    int month;
    QTimeZone timeZone;
};
```

建议的成功结果：

```cpp
struct MonthlyOverview
{
    std::vector<Transaction> transactions;
    Money totalIncome;
    Money totalExpense;
    Money balance;
};
```

其中：

```text
balance = totalIncome - totalExpense
```

金额始终使用 `Money`，不能为了统计方便改用 `double`。

## 6. 用例职责

`GetMonthlyOverview::execute()` 按以下顺序工作：

1. 验证年份、月份和时区有效。
2. 创建本地月份第一天零点。
3. 使用 `addMonths(1)` 得到下月第一天零点。
4. 把两个边界转换为 UTC。
5. 调用 `findOccurredBetween()`。
6. 查询失败时返回稳定的错误枚举。
7. 遍历账单，分别累加收入和支出。
8. 计算收入减支出的月结余。
9. 返回倒序流水与三个汇总金额。

用例不应该：

- 拼接 SQL。
- 读取 QML 控件。
- 把 Money 格式化成带货币符号的字符串。
- 假设所有用户都位于固定时区。

## 7. 为什么先在 Application 汇总

本课先查询当月流水，再在 Application 中累加，优点是：

- 业务规则可以用内存 Repository 快速测试。
- SQLite 与未来其他存储实现得到一致结果。
- 记账 MVP 的单月数据量很小，性能足够。

以后若流水规模很大，可以增加数据库聚合查询，但应先通过性能数据证明有必要，
并保留相同的业务语义。

## 8. SQLite 实现要求

`SqliteTransactionRepository` 必须：

- 拒绝无效时间或 `startInclusive >= endExclusive`。
- 将两个边界转换为 UTC ISO 8601 毫秒文本。
- 全部使用参数绑定。
- 依靠第十六课的 `idx_transactions_occurred_at` 索引。
- 按 `occurred_at DESC, id` 返回稳定顺序。
- 每一行仍通过 `Transaction::create()` 验证。
- SQL 失败时设置 `lastError()` 并返回 `std::nullopt`。

本课不需要数据库迁移版本 4，因为表结构没有变化。

## 9. 测试计划

### Repository 契约测试

- 开始时刻的账单被包含。
- 结束时刻的账单被排除。
- 范围外账单不返回。
- 返回结果按发生时间倒序。
- 无效或反向范围查询失败。
- 空范围查询成功并返回空 vector。

### GetMonthlyOverviewTest

- 空月份的三个金额都是零。
- 收入与支出分别正确累加。
- 月结余可以为正、零或负。
- 只汇总指定月份的账单。
- 上海时区月初边界转换正确。
- 跨年月份（12 月到次年 1 月）正确。
- 非法月份或无效时区返回 `InvalidInput`。
- Repository 失败返回 `StorageFailure`。

### SQLite 集成测试

- 时间范围参数正确绑定。
- UTC 文本边界查询正确。
- 真实数据库结果与内存 Repository 契约一致。

预计新增一个用例测试目标，总测试目标变为：

```text
15/15 tests passed
```

## 10. 文件变化计划

新增：

```text
src/features/accounting/application/GetMonthlyOverview.h
src/features/accounting/application/GetMonthlyOverview.cpp
tests/unit/accounting/GetMonthlyOverviewTest.cpp
```

修改：

```text
src/features/accounting/domain/ITransactionRepository.h
src/features/accounting/application/CMakeLists.txt
src/features/accounting/infrastructure/repositories/SqliteTransactionRepository.h
src/features/accounting/infrastructure/repositories/SqliteTransactionRepository.cpp
tests/unit/accounting/support/InMemoryTransactionRepository.h
tests/unit/accounting/TransactionRepositoryTest.cpp
tests/unit/accounting/CMakeLists.txt
tests/integration/accounting/SqliteTransactionRepositoryTest.cpp
```

## 11. 实施结果

本课使用固定 UTC+08:00 作为测试中的自定义时区，并验证本地 10 月对应的 UTC
边界为 9 月 30 日 16:00 到 10 月 31 日 16:00。代码没有调用
`QTimeZone::systemTimeZone()`，因此测试和业务结果不会跟随当前电脑设置变化。

查询接口使用 `std::optional<std::vector<Transaction>>` 区分空月份和存储错误，
SQLite 查询复用第十六课创建的时间索引，不需要新增数据库迁移。

## 12. 验收清单

- [x] Repository 提供含失败语义的时间范围查询。
- [x] 内存与 SQLite 实现遵守相同边界规则。
- [x] 月份边界按请求时区创建后转换为 UTC。
- [x] 查询使用左闭右开区间。
- [x] 月收入、月支出和月结余计算正确。
- [x] 空月份不是错误。
- [x] 非法月份和无效时区被拒绝。
- [x] SQLite 查询使用已有时间索引和参数绑定。
- [x] 15 组测试全部通过。
- [x] StuKit 应用仍能构建并启动。
- [x] 文档与代码导读同步更新。
- [x] Git 工作区整洁。

## 13. 完成本课后的下一步

完成第十七课后，记账模块将同时具备“写入一笔账”和“读取一个月账目”的核心能力。
第十八课可以开始建立 Presentation/ViewModel 层，把月度数据以 Qt 属性和列表模型
暴露给 QML；此时再设计页面，界面就能连接真实用例而不是静态占位数据。
