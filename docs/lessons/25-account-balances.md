# 第二十五课：账户余额与总资产

## 本课定位

第二十四课已经让账户可以创建、重命名、归档和恢复。账户目前保存了期初余额，
但首页只显示当月收支与结余，还没有回答“每个账户现在有多少钱”。本课已经实现：

```text
期初余额 + 该账户全部收入 - 该账户全部支出 = 当前余额
```

余额是派生数据，不增加 `accounts.balance` 字段，避免修改或删除账单后出现两份
余额不一致。

## 1. 本课目标

- 为每个账户计算当前余额。
- 汇总全部未归档账户的总资产。
- 账户管理列表显示当前余额，同时保留期初余额说明。
- 归档账户仍可计算和显示自身余额，但默认不计入“可用总资产”。
- 账单新增、编辑、删除后余额立即刷新。
- 金额计算继续只使用整数分 `Money`，不引入浮点数。
- 使用 Application 用例组装结果，QML 不访问 Repository。

## 2. 业务口径

第一版“当前余额”统计账户的全部历史流水，不受首页当前月份影响：

- 收入增加指定账户余额。
- 支出减少指定账户余额。
- 期初余额可以为正、零或负。
- 归档不会改变历史余额，也不会删除流水。
- 总资产只累加未归档账户，避免停用账户继续占据日常总览。

如果以后要支持“截至某日余额”，会在请求中显式增加截止时刻，而不是修改本课的
当前余额定义。

## 3. 数据结构

Application 层新增：

```text
AccountBalanceItem
  Account account
  Money currentBalance

AccountBalances
  vector<AccountBalanceItem> accounts
  Money activeTotal

GetAccountBalances
  -> IAccountRepository::findAll()
  -> ITransactionRepository::findAll()
```

用例一次读取全部账户和流水，在内存中按账户 UUID 累加。这样不会为每个账户发出
一次 SQL 查询，也不需要新增数据库迁移。

## 4. Presentation 与界面

账户余额列表模型向 QML 提供：

```text
accountId
name
openingBalanceText
currentBalanceText
archived
```

记账首页会显示“总资产”，账户管理列表显示“当前余额”和“期初余额”。余额刷新
通过组合根连接现有账单变更信号，不让 QML 自己计算。

## 5. 测试计划

### Application

- 无流水时当前余额等于期初余额。
- 多账户的收入和支出只影响对应账户。
- 正负期初余额均正确计算。
- 归档账户保留余额，但不计入活动总资产。
- 账户或流水查询失败返回稳定错误。
- 流水引用缺失账户时拒绝返回不完整结果。

### Presentation 与集成

- 金额格式始终保留两位小数。
- 新增、编辑、删除账单后余额同步变化。
- 重命名与归档后模型同步刷新。
- 真实 SQLite 重启后余额由持久化流水重新推导一致。
- QML 启动无类型、属性或绑定错误。

## 6. 预计文件变化

```text
src/features/accounting/application/GetAccountBalances.h/.cpp
src/features/accounting/presentation/AccountBalanceListModel.h/.cpp
src/features/accounting/presentation/AccountBalancesViewModel.h/.cpp
src/features/accounting/qml/AccountingHomePage.qml
src/app/AccountingModule.h/.cpp
相关单元测试与集成测试
```

## 7. 验收清单

- [x] 每个账户当前余额计算正确。
- [x] 活动账户总资产计算正确。
- [x] 归档账户不计入总资产但仍可查看余额。
- [x] 账单增删改后余额立即刷新。
- [x] 不新增重复余额字段或数据库迁移。
- [x] QML 不访问 Repository 或自行累计 Money。
- [x] 真实 SQLite 重启验证通过。
- [x] 全量测试与 QML 启动检查通过。
- [x] 文档与代码导读同步更新。

## 8. 实现补充

### Repository 查询失败不能伪装成空列表

原 `ITransactionRepository::findAll()` 只返回 `vector`，SQLite 读取失败时只能返回
空列表，会被误认为“没有账单”。本课将返回值统一为：

```cpp
std::optional<std::vector<Transaction>>
```

有值的空 vector 表示查询成功但没有流水；`nullopt` 表示读取失败或数据无效。

### 金额溢出保护

`GetAccountBalances` 在执行每次账户累加和总资产累加前检查 `qint64` 上下界。
如果极端金额会溢出，返回 `ArithmeticOverflow`，Presentation 显示稳定错误，避免
有符号整数回绕或未定义行为。

## 9. 实现与验证结果

- `GetAccountBalances` 一次读取全部账户和全部流水，按账户 UUID 在内存中累计。
- 无流水账户的当前余额等于期初余额，收入增加、支出减少对应账户余额。
- 已归档账户仍保留并展示余额，但不计入活动总资产。
- `AccountBalancesViewModel` 提供格式化总资产和账户余额模型，不向 QML 暴露 Money。
- 首页新增“总资产”卡片；管理账户列表同时显示当前余额、期初余额和归档状态。
- 基础资料及账单的新增、更新、删除信号都会驱动余额重新计算。
- 真实 SQLite 重启后不读取缓存余额，而是从期初余额与持久化流水重新推导。

验证结果：

```text
MSVC 2022 Release build: passed
CTest: 23/23 passed
QML lint: passed with no warnings
Windows QML startup smoke test: passed (no stderr output)
```

## 10. 后续方向

第二十六课将为当前月份的流水增加收支类型、账户、分类和备注关键字筛选。第一版
只筛选列表，不改变顶部完整月份汇总，避免“月度总计”和“当前筛选小计”含义混淆。
