# 第十六课：SQLite Transaction Repository 与迁移版本 3

## 本课定位

本课属于数据库实现阶段，由 Codex 负责代码、测试、排错、文档和提交。学习者可以
在完成记账 MVP 后回来复盘数据库设计。

本课将第十五课的新增账单流程连接到真实 SQLite：

```text
CreateTransaction
  -> ITransactionRepository
  -> SqliteTransactionRepository
  -> transactions 表
```

## 1. 本课目标

- 将数据库结构从版本 2 升级到版本 3。
- 创建 `transactions` 表和必要索引。
- 使用外键关联 `accounts` 与 `categories`。
- 实现 `ITransactionRepository` 的 SQLite 版本。
- 稳定保存金额、类型、关联 UUID、时间和备注。
- 验证版本 1、版本 2 和全新数据库都能升级到版本 3。
- 使用真实 SQLite 验证 `CreateTransaction` 完整调用链。

本课不制作流水页面，不实现删除账单，也不实现月度统计。

## 2. 为什么 transactions 必须是迁移 3

已发布迁移的职责不能回头修改：

```text
迁移 1：accounts
迁移 2：categories
迁移 3：transactions + indexes
```

升级路径：

```text
版本 0 -> 迁移 1 -> 迁移 2 -> 迁移 3
版本 1 ----------> 迁移 2 -> 迁移 3
版本 2 --------------------> 迁移 3
版本 3 --------------------> 不执行迁移
```

## 3. transactions 表设计

```sql
CREATE TABLE transactions (
    id TEXT PRIMARY KEY NOT NULL,
    type INTEGER NOT NULL CHECK (type IN (0, 1)),
    amount_cents INTEGER NOT NULL CHECK (amount_cents > 0),
    account_id TEXT NOT NULL,
    category_id TEXT NOT NULL,
    occurred_at TEXT NOT NULL,
    note TEXT NOT NULL DEFAULT '',
    FOREIGN KEY (account_id) REFERENCES accounts(id)
        ON UPDATE CASCADE ON DELETE RESTRICT,
    FOREIGN KEY (category_id) REFERENCES categories(id)
        ON UPDATE CASCADE ON DELETE RESTRICT
)
```

索引：

```sql
CREATE INDEX idx_transactions_occurred_at
ON transactions(occurred_at);

CREATE INDEX idx_transactions_account_id
ON transactions(account_id);

CREATE INDEX idx_transactions_category_id
ON transactions(category_id);
```

这些索引为后续月份筛选、账户筛选和分类筛选预留基础，但不提前加入尚未使用的复杂
组合索引。

## 4. 字段映射

| Domain | SQLite | 规则 |
| --- | --- | --- |
| `QUuid` | `TEXT` | 使用无花括号 UUID。 |
| `TransactionType` | `INTEGER` | Income=0，Expense=1。 |
| `Money` | `INTEGER` | 保存整数分，必须大于 0。 |
| `accountId` | `TEXT` | 外键引用 accounts.id。 |
| `categoryId` | `TEXT` | 外键引用 categories.id。 |
| `QDateTime` | `TEXT` | 转为 UTC 后使用 ISO 8601 毫秒格式。 |
| `QString note` | `TEXT` | 保存 Domain 已清理的备注。 |

时间统一存 UTC，避免 Windows 与 Android 处于不同时区时，同一笔账单产生不同排序
结果。读取后得到的 `QDateTime` 表示同一时刻，Presentation 再转换为本地显示时间。

## 5. 文件变化

新增：

```text
src/features/accounting/infrastructure/repositories/SqliteTransactionRepository.h
src/features/accounting/infrastructure/repositories/SqliteTransactionRepository.cpp
tests/integration/accounting/SqliteTransactionRepositoryTest.cpp
```

修改：

```text
src/features/accounting/infrastructure/database/SqliteDatabase.h
src/features/accounting/infrastructure/database/SqliteDatabase.cpp
src/features/accounting/infrastructure/CMakeLists.txt
tests/integration/accounting/SqliteDatabaseTest.cpp
tests/integration/accounting/CMakeLists.txt
```

## 6. 迁移 3 的事务边界

`applyMigration3()` 必须在一个事务中完成：

1. 创建 `transactions` 表。
2. 创建三个索引。
3. 写入 `schema_migrations` 版本 3。
4. 全部成功后提交。
5. 任一步失败都回滚。

迁移不能留下“表已创建但索引或版本记录缺失”的半完成状态。

## 7. 外键行为

SQLite 外键由 `SqliteDatabase::open()` 中的：

```sql
PRAGMA foreign_keys = ON
```

按连接启用。测试必须验证：

- 不存在的账户 UUID 无法写入账单。
- 不存在的分类 UUID 无法写入账单。
- 已被账单引用的账户和分类不能直接删除。

Application 已经提供友好的存在性检查，数据库外键则作为最后一道一致性保护。两层
校验并不重复：Application 负责业务错误，数据库负责防止任何代码路径写入孤儿数据。

## 8. SqliteTransactionRepository 职责

```text
save(transaction)   参数绑定 + UPSERT
findById(id)         找不到返回 std::nullopt
findAll()            occurred_at 降序、id 稳定排序
lastError()          保存数据库诊断信息
```

从数据库读取时必须：

1. 验证 type 只能是 0 或 1。
2. 用 `Money::fromCents()` 恢复金额。
3. 使用 `Qt::ISODateWithMs` 解析时间并确认有效。
4. 调用 `Transaction::create()` 重新验证完整领域规则。

## 9. 测试计划

### SqliteDatabaseTest

- 全新数据库包含 transactions 表及全部字段。
- 三个索引存在。
- 迁移记录恰好包含版本 1、2、3。
- 重复迁移不会重复记录版本。
- 版本 1 数据库能连续升级到版本 3。
- 版本 2 数据库能升级到版本 3 并保留账户、分类数据。

### SqliteTransactionRepositoryTest

- 保存并读取支出账单。
- 保存并读取收入账单。
- 金额以整数分完整往返。
- 时间以 UTC ISO 8601 毫秒格式保存并恢复。
- 相同 UUID 保存时更新原行。
- 缺失 UUID 返回 `std::nullopt` 且不记录错误。
- `findAll()` 按发生时间倒序返回。
- 无效账户或分类外键写入失败。
- `CreateTransaction` 能通过三个 SQLite Repository 完成真实持久化。
- Repository 在数据库对象之前析构。

完成后新增 CTest 目标：

```text
accounting.sqlite_transaction_repository
```

总测试目标预计为：

```text
14/14 tests passed
```

## 10. 本课不扩展 findAll 接口

`findAll()` 先提供稳定倒序结果。按月份、账户、分类查询将在流水列表用例中根据真实
界面需求设计，避免现在猜测一个过度复杂的通用过滤器。

下一阶段更可能新增明确接口，例如：

```text
findOccurredBetween(start, end)
```

而不是把所有查询条件堆入一个难以理解的函数。

## 11. 验收清单

- [x] 迁移 3 创建 transactions 表和三个索引。
- [x] 迁移 1、2 的历史职责不变。
- [x] 版本 1、2 数据库均可安全升级到版本 3。
- [x] 旧账户和分类数据升级后保持不变。
- [x] 金额使用 INTEGER 且数据库约束大于 0。
- [x] 时间统一保存为 UTC ISO 8601 毫秒文本。
- [x] 账户和分类使用外键并启用约束。
- [x] 收支类型使用显式稳定编码。
- [x] `SqliteTransactionRepository` 实现完整接口。
- [x] 查询结果重新经过 `Transaction::create()`。
- [x] `CreateTransaction -> SQLite` 集成测试通过。
- [x] 14 组测试全部通过。
- [x] StuKit 应用仍能构建并启动。
- [x] 文档与代码导读同步更新。
- [x] Git 工作区没有临时数据库或构建产物。

## 12. 实施方式

本课由 Codex 负责：

1. 实现事务化迁移 3。
2. 实现 SQLite Transaction Repository。
3. 编写升级、外键、映射和用例集成测试。
4. 运行完整构建和 14 组测试。
5. 运行应用启动冒烟测试。
6. 更新文档并创建独立 Git 提交。

完成第十六课后，下一步将进入按月份查询流水和月度汇总用例，为记账首页准备真实
数据能力。
