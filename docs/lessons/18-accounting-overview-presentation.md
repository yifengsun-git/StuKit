# 第十八课：记账首页 ViewModel 与流水列表模型

## 本课定位

前十七课已经完成记账核心规则、SQLite 持久化和月度汇总。本课建立 C++
Presentation 层，把 Application 结果转换成 QML 能直接绑定的页面状态：

```text
QML（下一课接入）
  -> AccountingOverviewViewModel
  -> GetMonthlyOverview
  -> ITransactionRepository

AccountingOverviewViewModel
  -> TransactionListModel
  -> QML ListView
```

本课不打开正式数据库，也不修改记账 QML 页面。这样 ViewModel 可以先用内存
Repository 完整测试。

## 1. 本课目标

- 建立独立 `StuKitAccountingPresentation` 静态库。
- 使用 `QObject`、`Q_PROPERTY`、signal 和 `Q_INVOKABLE` 暴露页面状态。
- 使用 `QAbstractListModel` 暴露月度流水。
- 支持上一月、下一月和重新加载。
- 支持地区时区和自定义固定偏移时区。
- 把 Money 格式化为两位小数文本，且不使用浮点数。
- 把 Application 错误转换为用户可理解的提示。
- 在不启动 QML 的情况下测试 Presentation。

## 2. 为什么不让 QML 直接调用用例

如果 QML 直接接触 Repository 或 Application 结果，它就必须理解：

- `std::variant` 成功与错误分支。
- `Money` 和 `Transaction` C++ 类型。
- 时区与年月参数。
- 空数据、数据库失败和重试状态。

ViewModel 把这些技术细节变成稳定属性，QML 只负责布局、颜色、动画和点击事件。

## 3. MoneyFormatter

新增：

```text
src/features/accounting/presentation/MoneyFormatter.h
src/features/accounting/presentation/MoneyFormatter.cpp
```

示例：

```text
0 分       -> 0.00
5 分       -> 0.05
-5 分      -> -0.05
12345 分   -> 123.45
```

实现使用整数的商和余数，不能先除以 `100.0`。这保证金额显示不会重新引入二进制
浮点误差，并覆盖 `qint64` 最小值的绝对值溢出边界。

当前不添加货币符号，因为“人民币还是其他货币”属于尚未确定的产品设置。

## 4. TransactionListModel

新增：

```text
src/features/accounting/presentation/TransactionListModel.h
src/features/accounting/presentation/TransactionListModel.cpp
```

模型角色：

| 角色 | QML 类型 | 说明 |
| --- | --- | --- |
| `transactionId` | string | 无花括号 UUID。 |
| `isIncome` | bool | true 为收入，false 为支出。 |
| `amountText` | string | 固定两位小数。 |
| `accountId` | string | 账户 UUID，后续用于跳转或查名。 |
| `categoryId` | string | 分类 UUID。 |
| `occurredAt` | date | 原始发生时刻。 |
| `note` | string | 备注文本。 |

模型通过 `beginResetModel()` 和 `endResetModel()` 整体替换一个月的数据。MVP 的
单月流水数量较小，这比过早实现复杂差量更新更可靠。

## 5. AccountingOverviewViewModel

新增：

```text
src/features/accounting/presentation/AccountingOverviewViewModel.h
src/features/accounting/presentation/AccountingOverviewViewModel.cpp
```

主要只读属性：

```text
selectedYear, selectedMonth, monthLabel
timeZoneId
totalIncomeText, totalExpenseText, balanceText
empty, hasError, errorMessage
transactions
```

QML 可调用操作：

```text
load()
showPreviousMonth()
showNextMonth()
selectTimeZone(id)
selectFixedOffsetMinutes(minutes)
```

月份或时区改变后，ViewModel 自动重新执行 `GetMonthlyOverview` 并更新模型与汇总。

## 6. 自定义时区规则

ViewModel 不调用 `QTimeZone::systemTimeZone()`。时区由构造函数显式注入，之后可以：

- 用 `selectTimeZone("Asia/Shanghai")` 选择地区时区。
- 用 `selectFixedOffsetMinutes(480)` 选择 UTC+08:00。

固定偏移限制在 UTC-14:00 到 UTC+14:00。无效时区不会替换当前有效设置，并显示
“时区无效”。设置的磁盘持久化属于下一课应用组合阶段。

## 7. 错误映射

Application 返回稳定错误枚举，Presentation 再决定中文提示：

```text
InvalidInput   -> 月份或时区无效
StorageFailure -> 无法读取账单数据
```

加载失败时清空旧列表和旧汇总，避免页面继续展示已经过期的数据。

## 8. CMake 依赖

```text
StuKitAccountingPresentation
  -> Qt6::Core
  -> StuKitAccountingApplication
  -> StuKitAccountingDomain（间接）
```

Presentation 不链接 `Qt6::Sql`，因此无法直接执行 SQL。这是用构建关系保护架构边界。

## 9. 自动测试

`AccountingPresentationTest.cpp` 验证：

- 正数、负数、零和 `qint64` 最小值的金额格式。
- 列表行数、角色名和角色值。
- 加载月度汇总并发出更新信号。
- 上一月、下一月会重新加载对应数据。
- 从 UTC 切换到自定义 UTC+08:00 后，月初边界流水正确进入当月。
- 无效地区时区和超范围固定偏移被拒绝。
- 存储错误可显示并在重试成功后清除。

本课完成后总测试目标：

```text
16/16 tests passed
```

## 10. 验收清单

- [x] 建立独立 Presentation 构建目标。
- [x] QML 可用属性、信号和操作已经声明。
- [x] 流水通过 `QAbstractListModel` 暴露。
- [x] ViewModel 不引用 SQLite 类型。
- [x] 金额格式化不使用浮点数。
- [x] 月份导航会重新加载数据。
- [x] 支持地区时区和固定偏移自定义时区。
- [x] 无效时区不会破坏当前状态。
- [x] 错误状态可以显示和恢复。
- [x] 16 组测试全部通过。
- [x] StuKit 应用仍能构建并启动。
- [x] Git 工作区不包含临时测试文件。

## 11. 本课完成后的状态

ViewModel 目前只在自动测试中连接内存 Repository。它尚未成为 QML 的全局对象，
也没有打开用户数据库，因此现有记账页面仍是占位页。这是有意的阶段边界，不是
遗漏。

第十九课将在应用启动时组装真实对象，并把 ViewModel 注入 QML，形成第一条真实的
端到端读取链路。
