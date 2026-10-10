# 第二十一课：新增账单表单

## 本课定位

第二十课已经允许用户创建真实账户和收支分类。本课计划完成第一笔真实账单的输入
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

计划提供的主要状态：

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

## 8. QML 交互计划

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

`AccountingModule` 计划增加并持有：

- `CreateTransaction`
- `CreateTransactionViewModel`

它复用已有三个 SQLite Repository、`AccountingSetupViewModel` 的基础数据，以及
`AccountingOverviewViewModel` 的当前时区。对象生命周期仍由组合根统一管理。

## 10. 测试计划

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

## 11. 预计文件变化

可能新增：

```text
src/features/accounting/presentation/CreateTransactionViewModel.h/.cpp
src/features/accounting/presentation/SelectableAccountListModel.h/.cpp
src/features/accounting/presentation/SelectableCategoryListModel.h/.cpp
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

模型是否单独拆分会在实现时以职责清晰和最少重复为准，不为了匹配预计文件名而制造
空壳类型。

## 12. 验收清单

- [ ] 没有基础数据时给出明确引导。
- [ ] 表单只显示未归档账户和类型匹配分类。
- [ ] 金额不经过浮点数。
- [ ] 时间使用用户自定义记账时区。
- [ ] QML 不创建 UUID、不访问 Repository。
- [ ] 创建成功后当前月汇总与列表立即更新。
- [ ] 保存失败时用户输入不丢失。
- [ ] 真实 SQLite 重启验证通过。
- [ ] 全量测试通过。
- [ ] 文档与代码导读同步更新。
