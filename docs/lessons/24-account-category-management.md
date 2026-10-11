# 第二十四课：账户与分类管理

## 本课定位

记账模块此前已经完成账单的新增、读取、编辑和删除，但账户与分类还只有
“新增”入口。本课已经建立基础资料管理页，让用户可以重命名、归档和恢复已有账户
与分类。

```text
AccountingHomePage / Management Dialog
  -> AccountingSetupViewModel
  -> UpdateAccount / UpdateCategory
  -> IAccountRepository / ICategoryRepository
  -> SQLite Repositories
```

为了保护历史账单，本课使用归档而不是物理删除。

## 1. 本课目标

- 提供账户与分类的统一管理入口。
- 列表同时显示活动项和已归档项，状态表达不只依赖颜色。
- 账户允许修改名称，并保留原 UUID 和期初余额。
- 分类允许修改名称，保留原 UUID 和收支类型。
- 账户与分类可以归档，也可以恢复。
- 归档项不再出现在新增或编辑账单的可选列表中。
- 历史账单仍显示归档项的名称，不破坏已有记录。
- 更新成功后同步刷新首次设置状态、账单表单选项和月度流水。

## 2. 为什么使用归档

账户和分类已被账单通过外键引用。物理删除会有两种不理想结果：

- 数据库拒绝删除，用户只看到失败。
- 如果改成级联删除，历史账单会被一起破坏。

归档会保留实体和历史引用，只把它从未来的新账单选项中移除。这与现有
`archived` 领域字段和数据库字段一致，不需要新迁移。

## 3. Application 用例

本课新增：

```text
UpdateAccount
UpdateCategory
```

`UpdateAccount` 接收账户 UUID，以及可选的名称和归档状态，先读取原账户，并
保留未修改字段和原期初余额。`UpdateCategory` 使用相同的补丁式请求，保留原
收支类型。重命名不会覆盖归档状态，归档也不会用界面旧值覆盖名称。

第一版不允许修改分类的收支类型。将已有“支出”历史的分类改成“收入”，
会让旧账单语义变得混乱；需要新类型时应新建分类。

## 4. Presentation 状态

`AccountingSetupViewModel` 已经持有完整账户和分类模型，本课增加：

- 重命名账户与分类的 `Q_INVOKABLE` 操作。
- 归档或恢复操作。
- 对空 UUID、无效名称、不存在和存储失败的中文错误映射。
- 成功后的 `setupChanged` 信号，继续驱动表单选项刷新。

组合根还把基础数据变更连接到 `AccountingOverviewViewModel::load()`，这样重命名后
流水列表会立即显示新名称。

## 5. QML 管理界面

常驻新增按钮附近新增了“管理账户与分类”入口。管理对话框分为：

- 账户列表：名称、期初余额、活动/已归档状态。
- 支出分类和收入分类：名称、类型、状态。
- 每行提供“重命名”与“归档/恢复”操作。

归档是会影响后续记账的操作，页面会在执行前给出明确确认。恢复不需要
二次确认。

## 6. 测试计划

### Application

- 账户重命名保留 UUID 和期初余额。
- 分类重命名保留 UUID 和收支类型。
- 归档与恢复正确持久化。
- 不存在和存储失败返回不同错误。

### Presentation

- 管理模型包含活动和已归档项。
- 更新后重新读取 Repository，不仅修改内存模型。
- 归档后账单表单不再显示该选项，恢复后重新显示。
- 账户或分类缺失时给出可理解错误。

### SQLite 与 QML

- 重启模块后保留名称和归档状态。
- 归档项的历史账单仍能显示关联名称。
- QML 管理界面无类型、属性或绑定错误。

## 7. 预计文件变化

```text
src/features/accounting/application/UpdateAccount.h/.cpp
src/features/accounting/application/UpdateCategory.h/.cpp
src/features/accounting/presentation/AccountingSetupViewModel.h/.cpp
src/features/accounting/qml/AccountingHomePage.qml
src/app/AccountingModule.h/.cpp
相关单元测试与集成测试
```

预计不修改数据库结构，现有 `archived` 字段已经能表达状态。

## 8. 验收清单

- [x] 可查看全部账户与分类及其状态。
- [x] 可重命名账户，且 UUID 和期初余额不变。
- [x] 可重命名分类，且 UUID 和收支类型不变。
- [x] 可归档和恢复账户与分类。
- [x] 归档前需要确认，恢复可直接执行。
- [x] 归档项不再出现在可选账单表单中。
- [x] 历史账单仍显示归档项名称。
- [x] 重命名后流水列表立即更新。
- [x] QML 不访问 Repository 或 SQL。
- [x] 真实 SQLite 重启验证通过。
- [x] 全量测试与 QML 启动检查通过。
- [x] 文档与代码导读同步更新。

## 9. 实现与验证结果

- 两个更新用例使用可选字段表达“只修改名称”或“只修改归档状态”。
- 账户更新保留 UUID 和期初余额；分类更新保留 UUID 和收支类型。
- ViewModel 为 QML 提供四个明确操作，并统一映射无效 UUID、名称、不存在和
  存储失败错误。
- 管理对话框分别列出全部账户与分类，文字显示“使用中/已归档”状态。
- 归档需要确认，恢复直接执行；成功后重新读取真实 Repository。
- `setupChanged` 同时刷新账单表单和月度流水，归档项不再可选，重命名立即显示。
- SQLite 集成测试证明重启后名称与归档状态仍存在，归档项的历史流水仍可读取。

验证结果：

```text
MSVC 2022 Release build: passed
CTest: 21/21 passed
Windows QML startup smoke test: passed (no stderr output)
```

## 10. 后续方向

第二十五课将计算账户当前余额与总资产。余额是由期初余额和流水推导出的结果，
不会增加一个容易失去同步的 `balance` 数据库字段。
