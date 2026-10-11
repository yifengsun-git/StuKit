# 第二十四课：账户与分类管理

## 本课定位

记账模块现在已经完成账单的新增、读取、编辑和删除，但账户与分类还只有
“新增”入口。本课将建立基础资料管理页，让用户可以重命名、归档和恢复已有账户
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

计划新增：

```text
UpdateAccount
UpdateCategory
```

`UpdateAccount` 接收账户 UUID、名称和归档状态，先读取原账户，并保留原期初
余额。`UpdateCategory` 接收分类 UUID、名称和归档状态，保留原收支类型。

第一版不允许修改分类的收支类型。将已有“支出”历史的分类改成“收入”，
会让旧账单语义变得混乱；需要新类型时应新建分类。

## 4. Presentation 状态

`AccountingSetupViewModel` 已经持有完整账户和分类模型，本课将增加：

- 重命名账户与分类的 `Q_INVOKABLE` 操作。
- 归档或恢复操作。
- 对空 UUID、无效名称、不存在和存储失败的中文错误映射。
- 成功后的 `setupChanged` 信号，继续驱动表单选项刷新。

组合根还会把基础数据变更连接到 `AccountingOverviewViewModel::load()`，这样重命名后
流水列表会立即显示新名称。

## 5. QML 管理界面

计划在常驻新增按钮附近增加“管理账户与分类”入口。管理对话框分为：

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

- [ ] 可查看全部账户与分类及其状态。
- [ ] 可重命名账户，且 UUID 和期初余额不变。
- [ ] 可重命名分类，且 UUID 和收支类型不变。
- [ ] 可归档和恢复账户与分类。
- [ ] 归档前需要确认，恢复可直接执行。
- [ ] 归档项不再出现在可选账单表单中。
- [ ] 历史账单仍显示归档项名称。
- [ ] 重命名后流水列表立即更新。
- [ ] QML 不访问 Repository 或 SQL。
- [ ] 真实 SQLite 重启验证通过。
- [ ] 全量测试与 QML 启动检查通过。
- [ ] 文档与代码导读同步更新。
