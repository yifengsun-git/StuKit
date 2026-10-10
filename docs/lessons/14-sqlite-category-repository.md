# 第十四课：SQLite Category Repository 与迁移版本 2

## 本课定位

本课属于数据库实现阶段，由 Codex 负责代码、测试、排错和提交。学习者现在不需要
亲手编写 SQL，可以在后续熟悉 SQLite 后回来复盘本文。

本课把第十三课的抽象接口连接到真实数据库：

```text
CreateCategory
  -> ICategoryRepository
  -> SqliteCategoryRepository
  -> categories 表
```

## 1. 本课目标

- 将数据库结构从版本 1 升级到版本 2。
- 新增 `categories` 表。
- 实现 `ICategoryRepository` 的 SQLite 版本。
- 完整保存 UUID、名称、收支类型和归档状态。
- 使用参数绑定和 UPSERT。
- 验证新数据库能连续应用迁移 1 和迁移 2。
- 验证已有版本 1 数据库能安全升级到版本 2。
- 使用真实 SQLite 验证 `CreateCategory` 完整调用链。

本课不实现分类页面，也不处理分类重名规则。

## 2. 为什么要创建迁移版本 2

第十一课发布的数据库版本 1 只包含：

```text
accounts
schema_migrations(version = 1)
```

已经存在的用户数据库不能通过修改迁移 1 来升级。发布过的迁移必须保持不变，新的
结构变化只能追加迁移 2：

```text
版本 0 --迁移 1--> accounts
       --迁移 2--> categories

版本 1 --迁移 2--> categories

版本 2 ----------> 不重复执行任何迁移
```

这样 Windows 旧版本用户和未来 Android 用户都能安全升级已有数据库。

## 3. categories 表设计

```sql
CREATE TABLE categories (
    id TEXT PRIMARY KEY NOT NULL,
    name TEXT NOT NULL,
    type INTEGER NOT NULL CHECK (type IN (0, 1)),
    archived INTEGER NOT NULL DEFAULT 0 CHECK (archived IN (0, 1))
)
```

字段映射：

| C++ | SQLite | 说明 |
| --- | --- | --- |
| `QUuid` | `TEXT` | 保存为无花括号 UUID。 |
| `QString` | `TEXT` | 保存已经通过 Domain 校验的名称。 |
| `TransactionType::Income` | `INTEGER 0` | 数据库编码显式定义。 |
| `TransactionType::Expense` | `INTEGER 1` | 不直接依赖枚举底层值。 |
| `bool` | `INTEGER 0/1` | 通过 CHECK 约束限制。 |

这里暂不添加名称唯一约束，因为第十三课的 Application 契约允许同名分类。数据库
约束不能与业务层公开行为互相冲突。

## 4. 文件变化

新增：

```text
src/features/accounting/infrastructure/repositories/SqliteCategoryRepository.h
src/features/accounting/infrastructure/repositories/SqliteCategoryRepository.cpp
tests/integration/accounting/SqliteCategoryRepositoryTest.cpp
```

修改：

```text
src/features/accounting/infrastructure/database/SqliteDatabase.h
src/features/accounting/infrastructure/database/SqliteDatabase.cpp
src/features/accounting/infrastructure/CMakeLists.txt
tests/integration/accounting/SqliteDatabaseTest.cpp
tests/integration/accounting/CMakeLists.txt
```

## 5. 迁移版本 2 的职责

`SqliteDatabase` 将新增私有函数：

```cpp
[[nodiscard]] bool applyMigration2();
```

`migrate()` 按顺序检查版本：

```cpp
if (version < 1 && !applyMigration1()) {
    return false;
}

if (version < 2 && !applyMigration2()) {
    return false;
}
```

迁移 2 必须在同一个事务中完成：

1. 创建 `categories` 表。
2. 向 `schema_migrations` 写入版本 2。
3. 两步全部成功后提交。
4. 任意一步失败则回滚。

不能先提交建表再单独记录版本，否则程序可能留下“表存在但版本没有升级”的半完成
状态。

## 6. SqliteCategoryRepository 的职责

它实现第十三课定义的三个接口函数：

```text
save(category)   INSERT 或按相同 UUID UPDATE
findById(id)     返回 Category 或 std::nullopt
findAll()        按类型、名称、UUID 稳定排序
```

与账户 Repository 相同，它还提供：

```cpp
[[nodiscard]] const QString &lastError() const noexcept;
```

数据库读取结果必须重新调用：

```cpp
Category::create(...)
```

Repository 不能绕过 Domain 规则制造 Category。

## 7. 收支类型不能直接强制转换

虽然当前枚举通常会把第一个值表示为 0、第二个值表示为 1，但数据库格式不应该
依赖编译器中的枚举排列。实现中会使用两个显式转换函数：

```text
Income  <-> 0
Expense <-> 1
```

如果数据库出现其他数字，Repository 会报告“数据库包含无效分类”，而不是猜测或
默认为支出类型。

## 8. 测试计划

### 更新 SqliteDatabaseTest

验证：

- `categories` 表存在。
- 字段 `id`、`name`、`type`、`archived` 存在。
- 新数据库最终记录迁移版本 1 和 2。
- 重复调用 `migrate()` 不会重复写入版本。
- 模拟版本 1 数据库后可以升级到版本 2。

### 新增 SqliteCategoryRepositoryTest

验证：

- 保存并读取支出分类。
- 保存并读取收入分类。
- 相同 UUID 再次保存执行更新。
- 不存在的 UUID 返回 `std::nullopt` 且不记录错误。
- `findAll()` 返回全部分类。
- `CreateCategory` 能通过 SQLite Repository 持久化分类。
- Repository 在数据库对象之前析构。

完成后 CTest 将从 10 个目标增加到 11 个：

```text
accounting.sqlite_category_repository
```

迁移测试方法会增加，但仍属于已有的 `accounting.sqlite_database` 目标。

## 9. 关键安全边界

- 不修改迁移 1 的历史含义。
- SQL 中的名称和 UUID 全部通过 `bindValue()` 绑定。
- 迁移使用事务。
- 数据库约束与 Domain 规则保持一致。
- Repository 不生成 UUID，也不显示错误提示。
- Application 和 Domain 不引用 Qt SQL。
- 测试只使用临时数据库，不污染正式数据。
- 不把 Infrastructure 文件加入 QML 模块。

## 10. 验收清单

- [ ] `categories` 表只通过迁移 2 创建。
- [ ] 迁移 1 保持兼容。
- [ ] 新数据库连续记录版本 1 和版本 2。
- [ ] 已有版本 1 数据库可以升级。
- [ ] `type` 和 `archived` 都有 CHECK 约束。
- [ ] 收支类型使用显式数据库编码。
- [ ] `SqliteCategoryRepository` 实现 `ICategoryRepository`。
- [ ] 保存使用参数绑定和 UPSERT。
- [ ] 查询结果重新经过 `Category::create()`。
- [ ] `CreateCategory -> SQLite` 集成测试通过。
- [ ] 11 组测试全部通过。
- [ ] StuKit 应用仍能构建并启动。
- [ ] 文档与 C++ 代码导读同步更新。
- [ ] Git 工作区没有数据库文件或构建产物。

## 11. 实施方式

本课由 Codex 依次执行：

1. 修改迁移代码。
2. 编写 Repository。
3. 编写迁移和 Repository 集成测试。
4. 完整构建并运行全部测试。
5. 运行应用启动冒烟测试。
6. 解释遇到的错误与解决方法。
7. 更新文档并创建清晰 Git 提交。

学习者后续复盘时，建议先阅读迁移测试，再阅读迁移实现，最后阅读 Repository；
测试比 SQL 实现更容易说明代码想保证什么行为。
