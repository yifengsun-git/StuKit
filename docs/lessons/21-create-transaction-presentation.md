# 第二十一课：新增账单表单

## 本课定位

第二十课已经允许用户创建真实账户和收支分类。本课完成第一笔真实账单的输入
流程，并在保存成功后刷新当前月首页：

```text
AccountingHomePage / Transaction Dialog
  -> CreateTransactionViewModel
  -> CreateTransaction
  -> SqliteTransactionRepository
  -> AccountingOverviewViewModel::load()
```

QML 仍只负责收集输入和绑定状态，不直接查询 Repository，也不创建领域实体。

## 1. 本课目标

- 新增账单表单 ViewModel。
- 复用第二十课的账户和分类模型。
- 根据收入/支出类型只显示类型匹配的分类。
- 精确解析正数金额，拒绝零和负数。
- 将用户本地输入时间按自定义记账时区转换为真实时刻。
- 调用现有 `CreateTransaction` 用例保存到账单表。
- 保存成功后刷新月度汇总和流水列表。
- 为输入转换、错误映射和 SQLite 完整链路补充测试。

## 2. 为什么不让 QML 直接调用 CreateTransaction

表单需要协调金额文本、下拉选择、时间、备注和错误提示。若全部写在 QML 中，页面
会开始承担业务编排，并难以用 Qt Test 独立验证。因此新增
`CreateTransactionViewModel` 作为 QML 与 Application 用例之间的桥梁。

调用方向保持为：

```text
QML -> Presentation ViewModel -> Application -> Domain Repository interface
```

## 3. 表单字段

第一版表单包含：

- 类型：支出或收入。
- 金额：字符串输入，最多两位小数。
- 账户：从未归档账户中选择。
- 分类：从未归档且类型匹配的分类中选择。
- 发生日期与时间。
- 备注：可为空，长度规则继续由 Transaction 实体判断。

如果账户或对应类型分类不存在，新增按钮不会进入一个无法完成的表单，而是提示
用户先完成第二十课的基础设置。

## 4. 金额规则

复用 `MoneyInputParser`，但账单金额还要满足领域规则：

```text
12      -> 接受，1200 分
12.34   -> 接受，1234 分
0       -> 拒绝
-12.34  -> 拒绝
12.345  -> 拒绝
```

解析器负责格式与溢出，`Transaction::create()` 仍负责“金额必须为正数”的最终领域
约束，避免表现层成为唯一防线。

## 5. 分类过滤

第二十课的分类模型含 `isIncome` 和 `archived` 角色。本课需要为表单提供可选择项：

- 支出账单：仅显示未归档支出分类。
- 收入账单：仅显示未归档收入分类。

优先在 C++ 模型或代理模型中完成过滤，不在 QML delegate 中简单隐藏行；隐藏行仍会
影响索引，容易把错误的 categoryId 传给用例。

## 6. 自定义时区与发生时间

用户输入的是记账时区中的“墙上时间”，数据库保存的是 UTC 时刻。转换流程为：

```text
表单日期 + 表单时间 + 用户自定义 QTimeZone
  -> QDateTime
  -> 验证有效且不存在无法解释的时间
  -> CreateTransactionRequest.occurredAt
  -> SQLite 中的 UTC ISO 8601 文本
```

不能直接使用设备系统时区，因为用户已经在第十九课明确选择了独立记账时区。地区
时区还可能遇到夏令时跳变或重复时间，测试需要覆盖至少一个边界。

## 7. CreateTransactionViewModel

提供的主要状态：

```text
isIncome, amountText, note
selectedAccountId, selectedCategoryId
occurredDate, occurredTime
availableAccounts, availableCategories
canSubmit, hasError, errorMessage
```

主要操作：

```text
resetForNewTransaction()
setTransactionType(isIncome)
submit()
```

ViewModel 将 `CreateTransactionError` 映射为稳定中文提示。保存成功后发出
`transactionCreated` 信号，由组合层或页面触发月度首页重新加载。

## 8. QML 交互

记账首页增加“记一笔”主按钮。对话框中使用：

- 收入/支出切换控件。
- 金额输入框。
- 账户 ComboBox。
- 分类 ComboBox。
- 日期和时间输入控件。
- 备注输入框。
- 取消和保存按钮。

保存失败时保留用户输入并显示错误；保存成功才关闭对话框。页面刷新后，新流水和
月度汇总应立即可见。

## 9. 组合根更新

`AccountingModule` 增加并持有：

- `CreateTransaction`
- `CreateTransactionViewModel`

它复用已有三个 SQLite Repository、`AccountingSetupViewModel` 的基础数据，以及
`AccountingOverviewViewModel` 的当前时区。对象生命周期仍由组合根统一管理。

## 10. 测试覆盖

### Presentation

- 空基础数据时不能提交。
- 收入/支出切换后分类列表正确过滤。
- 无效金额、零金额、负金额和溢出金额被拒绝。
- 无效 UUID 和已归档数据得到明确错误。
- 自定义固定时区转换正确。
- 保存失败后输入保留并可重试。
- 保存成功发出信号并清理表单。

### 真实 SQLite

- 通过组合根和 ViewModel 创建第一笔账单。
- 账单引用正确账户与分类。
- 模块重启后账单仍存在。
- 当前月汇总和流水列表在创建后更新。

## 11. 实际文件变化

新增：

```text
src/features/accounting/presentation/CreateTransactionViewModel.h/.cpp
tests/unit/accounting/CreateTransactionPresentationTest.cpp
```

需要修改：

```text
AccountingModule.h/.cpp
main.cpp
Main.qml
AccountingHomePage.qml
相关 CMake 与集成测试
```

没有新增 SelectableAccountListModel 或 SelectableCategoryListModel。表单 ViewModel
复用现有 `AccountListModel` 和 `CategoryListModel`，向模型放入已经过滤好的实体，
避免两个职责相同的模型类型。

## 12. 验收清单

- [x] 没有基础数据时给出明确引导。
- [x] 表单只显示未归档账户和类型匹配分类。
- [x] 金额不经过浮点数。
- [x] 时间使用用户自定义记账时区。
- [x] QML 不创建 UUID、不访问 Repository。
- [x] 创建成功后当前月汇总与列表立即更新。
- [x] 保存失败时用户输入不丢失。
- [x] 真实 SQLite 重启验证通过。
- [x] 全量测试通过。
- [x] 文档与代码导读同步更新。

## 13. 实现与验证结果

实现中的关键行为：

- 表单打开时重新读取基础数据，默认选择第一个有效账户。
- 优先使用支出分类；只有收入分类时自动切换到收入模式。
- 收支切换会在 C++ 中重建分类模型，不在 QML 中隐藏不匹配的行。
- 日期和时间以 `QDateTime::TransitionResolution::Reject` 结合记账时区构造，夏令时
  缺口或重复时间不会被静默解释成另一时刻。
- `transactionCreated` 信号由组合根连接到月度首页 `load()`，保存后立即刷新汇总。
- 保存失败不会清空表单；保存成功后才清理金额与备注并关闭对话框。
- 账户或分类发生变化时，表单选项通过 `setupChanged` 自动重新加载。

验证结果：

```text
MSVC 2022 Release build: passed
CTest: 19/19 passed
Windows QML startup smoke test: passed
```

测试覆盖固定 UTC+08:00 转换、`America/New_York` 夏令时缺口、无效金额、真实
SQLite 写入、首页刷新和模块重启恢复。

## 14. 后续方向

第二十二课将完善流水的可读信息与详情展示。当前列表已经能显示金额、时间和备注，
下一步补到账户名称、分类名称和完整账单详情，为后续编辑、删除建立可靠入口。
