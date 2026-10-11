# 第二十三课：编辑与删除账单

## 本课定位

第二十二课已经让流水列表显示可读的账户名和分类名，并可以打开只读详情。
本课将从这个稳定入口扩展两项常用能力：修改已有账单和删除账单。

```text
AccountingHomePage / Transaction Dialog
  -> TransactionEditorViewModel
  -> UpdateTransaction / DeleteTransaction
  -> ITransactionRepository
  -> SqliteTransactionRepository
  -> AccountingOverviewViewModel::load()
```

编辑与删除是不同的业务操作，会使用明确的 Application 用例，不会让 QML
直接调用 Repository 或执行 SQL。

## 1. 本课目标

- 可以从账单详情进入编辑状态。
- 编辑时预填原有收支类型、金额、账户、分类、时间和备注。
- 更新时保留账单 UUID，不把修改错误实现为“删除后新建”。
- 复用新增账单的金额、分类、账户和时区校验。
- 删除前弹出二次确认，防止误操作。
- 编辑或删除成功后立即刷新当前月列表与汇总。
- 为 Application、Presentation、SQLite 和组合根补充回归测试。

## 2. 先定义存储契约

`ITransactionRepository` 已经有 `save()` 和 `findById()`，但还没有删除契约。
本课计划新增：

```cpp
virtual bool removeById(const QUuid &id) = 0;
```

或者使用能区分“成功”“不存在”“存储失败”的显式结果类型。实现时会先检查
当前 Repository 的错误表达方式，避免一个布尔值隐藏重要差异。

SQLite 实现使用带 UUID 条件的单行 `DELETE`，并检查受影响行数。不会删除
账户或分类，也不需要新的数据库迁移。

## 3. UpdateTransaction 用例

更新不能只依赖 Repository 的 UPSERT，否则调用者传入不存在的 UUID 时会意外创建
新账单。`UpdateTransaction` 计划执行：

1. 检查原账单是否存在。
2. 通过 `Transaction::create()` 重新验证全部字段。
3. 检查账户、分类是否存在且可用。
4. 检查分类收支类型与账单一致。
5. 保留原 UUID，通过 `save()` 更新数据。

这些步骤与 `CreateTransaction` 有重叠，实现时可以抽取小型共享校验函数，但不会
为了减少几行代码就合并两个含义不同的用例。

## 4. DeleteTransaction 用例

`DeleteTransaction` 接收账单 UUID，并返回稳定错误：

- UUID 无效。
- 账单不存在。
- 读取或删除时存储失败。

删除成功后不保留“软删除”记录。这是个人本地记账工具的第一版取舍；
如果后续增加同步、多设备冲突或操作历史，再单独设计软删除和审计字段。

## 5. 表单编辑模式

现有 `CreateTransactionViewModel` 已经承担了大部分表单状态。实现时会评估将它重命名为
`TransactionEditorViewModel`，或为编辑增加一个薄适配层。无论选择哪种方式，QML
都只看到统一表单状态：

```text
isEditing
editingTransactionId
submitButtonText
deleteAvailable
```

编辑时必须以账单保存的 UTC 时刻结合当前记账时区回填日期和时间，不能
直接使用设备系统时区。

## 6. QML 交互

- 详情对话框提供“编辑”和“删除”入口。
- 编辑复用新增表单，但标题、主按钮文案和默认值随模式变化。
- 删除需要单独确认对话框，明确告知该操作会永久删除记录。
- 用例失败时保留当前对话框和输入，显示可恢复的中文错误。
- 成功后关闭对话框，并通过组合根信号刷新首页。

## 7. 测试计划

### Application

- 更新保留 UUID 并修改所有允许字段。
- 更新不存在的账单不会变成新建。
- 账户、分类、收支类型和领域校验失败都返回正确错误。
- 删除成功、不存在和存储失败可区分。

### Presentation

- 新增与编辑模式初始状态正确。
- 原有字段和自定义时区时间回填正确。
- 提交更新和删除会发出刷新信号。
- 失败保留输入并显示明确错误。

### SQLite 与组合根

- 真实 SQLite 修改和删除可持久化。
- 列表、月收入、月支出和结余随操作刷新。
- 应用重启后保持最新状态。
- QML 启动无类型、属性或绑定错误。

## 8. 预计文件变化

```text
src/features/accounting/domain/repositories/ITransactionRepository.h
src/features/accounting/application/UpdateTransaction.h/.cpp
src/features/accounting/application/DeleteTransaction.h/.cpp
src/features/accounting/infrastructure/repositories/SqliteTransactionRepository.h/.cpp
src/features/accounting/presentation/CreateTransactionViewModel.h/.cpp
src/features/accounting/qml/AccountingHomePage.qml
src/app/AccountingModule.h/.cpp
相关单元测试与集成测试
```

如果表单 ViewModel 重命名，会同步修改 CMake 目标和代码导读。预计不新增数据库
迁移。

## 9. 验收清单

- [ ] 详情可进入编辑，且所有字段回填正确。
- [ ] 更新保留原账单 UUID。
- [ ] 更新不存在的 UUID 不会新建账单。
- [ ] 删除前必须二次确认。
- [ ] 编辑与删除成功后当月页立即刷新。
- [ ] 失败状态有稳定错误并不丢失输入。
- [ ] QML 不访问 Repository 或 SQL。
- [ ] 自定义时区、整数分金额和分类过滤不回退。
- [ ] 真实 SQLite 与应用重启验证通过。
- [ ] 全量测试与 QML 启动检查通过。
- [ ] 文档与代码导读同步更新。
