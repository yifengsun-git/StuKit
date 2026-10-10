# StuKit C++ 代码导读

## 1. 文档目的

本文说明 StuKit 中各个 C++ 头文件和源文件的职责、依赖关系以及不能承担的职责。
它适合在阅读代码、排查编译错误或添加新功能前快速查阅。

当前内容对应第十六课完成后的代码，包括三版 SQLite 迁移、三个 Repository 接口、
账户、分类与账单持久化，以及创建账户、分类和账单用例。

## 2. `.h` 和 `.cpp` 分别做什么

通常一个 C++ 类型由两个文件组成：

```text
Example.h    声明“这个类型提供什么能力”
Example.cpp  实现“这些能力具体怎么工作”
```

头文件会被其他源文件包含，因此应该尽量保持依赖清晰。实现细节尽量放在 `.cpp`
中，可以减少重新编译范围，也能避免调用者依赖内部实现。

有些文件只有 `.h`：

- 只包含枚举或很小的类型定义。
- 纯抽象接口没有需要单独编译的实现。
- 测试替身为了被多个测试直接复用而写成头文件实现。

## 3. 当前调用关系

```text
main.cpp
  -> QML 应用外壳

CreateAccount
  -> IAccountRepository
  -> Account
       -> Money

CreateTransaction
  -> ITransactionRepository
  -> IAccountRepository
  -> ICategoryRepository
  -> Transaction

InMemoryAccountRepository（仅测试）
  -> IAccountRepository

SqliteDatabase
  -> Qt SQL / QSQLITE
  -> schema_migrations 表
  -> accounts 表
  -> categories 表
  -> transactions 表与索引

SqliteAccountRepository
  -> IAccountRepository
  -> QSqlDatabase / accounts 表

SqliteCategoryRepository
  -> ICategoryRepository
  -> QSqlDatabase / categories 表

SqliteTransactionRepository
  -> ITransactionRepository
  -> QSqlDatabase / transactions 表

Transaction
  -> Money
  -> TransactionType

Category
  -> TransactionType
```

主要架构依赖方向：

```text
Presentation/QML -> Application -> Domain
Infrastructure --------------------> Domain
```

当前 QML 尚未调用创建用例。账户、分类和账单的 SQLite 存储已经完成，后续由
Presentation 层触发用例。

## 4. 程序入口

### `src/app/main.cpp`

作用：

- 创建 `QGuiApplication`，启动 Qt GUI 事件循环。
- 设置组织名和应用名，供设置、路径等 Qt API 使用。
- 创建 `QQmlApplicationEngine`。
- 从 QML 模块 `StuKit` 加载 `Main.qml`。
- 在 QML 根对象创建失败时，以错误码退出。

它是整个应用的启动入口，但不应该包含：

- 记账规则。
- SQL 查询。
- 页面布局。
- 金额计算。

以后需要组装 Repository、用例和 ViewModel 时，会把对象创建集中到应用组合入口，
但仍不会把业务规则直接写进 `main.cpp`。

## 5. Domain：记账领域层

目录：

```text
src/features/accounting/domain/
```

Domain 保存记账业务最核心的概念和规则，不依赖 QML、Qt Quick 或 Qt SQL。

### `Money.h`

声明金额值对象 `Money`：

- 内部使用 `qint64` 保存整数“分”。
- 通过 `fromCents()` 明确创建。
- 提供正数、负数和零判断。
- 提供加法、减法和相等比较。

Money 是值对象：两个 Money 的分值相同，就代表相同金额，不需要 UUID。

### `Money.cpp`

实现 Money 的创建、读取、判断和运算。

这里不负责把金额格式化成 `12.34 元`。显示格式属于 Presentation；数据库以后只
保存整数分。

### `TransactionType.h`

定义收支类型：

```cpp
enum class TransactionType
{
    Income,
    Expense
};
```

Transaction 和 Category 共享这个类型。独立头文件避免两个类分别定义不兼容的
收入、支出枚举。

它属于记账模块，不应放入全局 `shared/`，因为课表等其他模块不需要这个概念。

### `Transaction.h`

声明账单实体及创建数据：

- `TransactionData` 收集创建一笔账单所需的字段。
- `Transaction` 保存账单自身 ID、收支类型、金额、账户 ID、分类 ID、发生时间和
  备注。
- `create()` 是唯一公开创建入口。
- Getter 只读取数据，不允许调用者直接破坏实体状态。

Transaction 是实体：即使字段完全相同，只要 UUID 不同，就是两笔不同账单。

### `Transaction.cpp`

实现账单创建规则：

- 账单、账户和分类 UUID 都不能为空。
- 金额必须大于零。
- 发生时间必须有效。
- 备注会去除首尾空白。

它不负责确认账户或分类是否真的存在，也不负责确认分类类型是否与账单类型一致。
这些跨实体规则需要 Repository 参与，应由 Application 用例处理。

### `Account.h`

声明资金账户实体：

- `id`：账户 UUID。
- `name`：资金账户名称。
- `openingBalance`：开始记账时的期初余额。
- `archived`：账户是否归档。
- `MaximumNameLength`：名称最长 40 个字符。

这里的 Account 表示现金、银行卡、支付宝、信用卡等“资金账户”，不是登录账号。

### `Account.cpp`

实现资金账户创建规则：

- UUID 不能为空。
- 名称会去除首尾空白。
- 名称不能为空且不能超过 40 个字符。
- 期初余额允许为正数、零或负数。

它不保存实时余额。实时余额以后通过“期初余额 + 收入 − 支出”计算，避免余额与
流水不一致。

### `Category.h`

声明收支分类实体：

- `id`：分类 UUID。
- `name`：分类名称。
- `type`：收入分类或支出分类。
- `archived`：分类是否归档。
- `MaximumNameLength`：名称最长 40 个字符。

例如“工资”属于收入分类，“餐饮”属于支出分类。

### `Category.cpp`

实现分类创建规则：

- UUID 不能为空。
- 名称去除首尾空白后不能为空。
- 名称不能超过 40 个字符。
- 保存传入的收支类型和归档状态。

分类名称是否重复需要查询其他分类，不是单个 Category 能独立判断的规则。

### `IAccountRepository.h`

声明资金账户的存储接口：

```text
save(account)   保存或更新账户
findById(id)    按 UUID 查找账户
findAll()       返回全部账户
```

这是纯抽象接口，不能直接创建对象。它描述 Application 需要的存储能力，但不说明
数据存在哪里。

该文件不能包含 `QSqlDatabase` 或 SQL 语句。SQLite Repository 将在
Infrastructure 层实现这个接口。

### `ICategoryRepository.h`

声明分类存储契约，提供保存、按 UUID 查找和返回全部分类的能力。它与
`IAccountRepository` 分开，使两个业务概念以后可以独立增加查询方法。

接口只依赖 Category 和 Qt Core 基础类型，不依赖 SQLite。单元测试使用内存实现，
正式持久化由 Infrastructure 中的 `SqliteCategoryRepository` 提供。

### `ITransactionRepository.h`

声明账单的最小存储契约：保存或更新、按 UUID 查询和返回全部账单。它当前不包含
月份筛选；查询能力会在实现流水列表时根据真实需求扩展。

接口属于 Domain，不引用 Qt SQL，也不负责检查关联账户和分类是否存在。

## 6. Application：应用用例层

目录：

```text
src/features/accounting/application/
```

Application 负责组织一次完整的用户目标。它可以调用实体和 Repository 接口，但
不处理页面控件和 SQL。

### `CreateAccount.h`

声明创建资金账户用例所需的类型：

- `CreateAccountRequest`：名称和期初余额。
- `CreateAccountError`：无效输入或存储失败。
- `CreateAccountResult`：成功时保存 Account，失败时保存错误枚举。
- `CreateAccount`：通过构造函数接收 `IAccountRepository`。

结果使用 `std::variant`，确保一次调用只可能是成功账户或一种错误，不会同时出现
互相矛盾的状态。

### `CreateAccount.cpp`

实现创建账户流程：

1. 生成新的 UUID。
2. 使用请求数据调用 `Account::create()`。
3. 输入无效时返回 `InvalidInput`。
4. 调用 Repository 保存有效账户。
5. 保存失败时返回 `StorageFailure`。
6. 成功时返回创建后的 Account。

它只依赖 `IAccountRepository`，所以测试可以传入内存实现，正式应用以后可以传入
SQLite 实现。

### `CreateCategory.h` 与 `CreateCategory.cpp`

定义并实现创建分类用例：

1. 接收分类名称和收入/支出类型。
2. 生成 UUID 并调用 `Category::create()`。
3. 将无效输入转换为 `InvalidInput`。
4. 通过 `ICategoryRepository` 保存分类。
5. 将保存失败转换为 `StorageFailure`。

它与 CreateAccount 遵守相同的 Application 层模式，但分别依赖各自 Repository，
没有为了少量重复而合并成含义模糊的通用用例。

### `CreateTransaction.h` 与 `CreateTransaction.cpp`

实现新增账单的跨实体流程：

1. 调用 `Transaction::create()` 验证金额、UUID、时间和备注。
2. 查询账户并拒绝不存在或已归档账户。
3. 查询分类并拒绝不存在或已归档分类。
4. 检查分类类型与账单收入/支出类型一致。
5. 通过 `ITransactionRepository` 保存有效账单。

结果错误枚举能区分每种失败原因，后续 Presentation 可以把稳定错误类型转换为
用户提示，而不用解析字符串。

## 7. Infrastructure：基础设施层

目录：

```text
src/features/accounting/infrastructure/
```

Infrastructure 负责数据库、文件和平台 API 等外部技术细节。它可以依赖 Domain，
但 Domain 不能反向依赖 Infrastructure。

### `database/SqliteDatabase.h`

声明 SQLite 数据库管理类：

- 使用唯一连接名持有一条 Qt SQL 连接。
- `open()` 打开数据库并启用外键约束。
- `migrate()` 将旧数据库结构升级到当前版本。
- `connection()` 为后续 Repository 提供数据库连接。
- 禁止复制和移动，避免一个连接被多个对象错误管理。

它管理连接和表结构，不负责账户的增删改查，也不包含界面逻辑。

### `database/SqliteDatabase.cpp`

实现数据库生命周期和顺序迁移：

1. 创建 `schema_migrations` 表记录已应用版本。
2. 查询当前最高版本。
3. 在事务中创建 `accounts` 表。
4. 成功后记录版本 1，失败则回滚。
5. 析构时关闭连接并从 Qt 连接池移除连接名。

迁移 2 在独立事务中创建 `categories` 表并记录版本 2。迁移 3 创建
`transactions` 表、三个查询索引和两个外键，并记录版本 3。每版迁移都在独立事务
中完成；全新数据库会依次执行迁移 1、2、3，旧数据库只执行缺失版本。

迁移具有幂等性：同一个数据库多次调用 `migrate()` 不会重复创建表。

### `repositories/SqliteAccountRepository.h`

声明 `IAccountRepository` 的 SQLite 实现：

- `save()` 保存或更新账户。
- `findById()` 按 UUID 查询账户。
- `findAll()` 返回稳定排序的全部账户。
- `lastError()` 暴露具体数据库诊断信息。

它持有的是 Qt 命名连接的轻量句柄，不拥有数据库文件，也不负责打开数据库和执行
迁移。

### `repositories/SqliteAccountRepository.cpp`

负责 Account 与数据库字段之间的双向转换：

- UUID 保存为无花括号文本。
- Money 保存为整数分。
- bool 保存为 0 或 1。
- SQL 使用参数绑定，账户名称不会被拼进 SQL 字符串。
- 相同 UUID 使用 SQLite UPSERT 更新原行。
- 查询结果重新通过 `Account::create()` 验证领域规则。

该文件不处理按钮、提示文案或创建 UUID；这些分别属于 Presentation 和
Application。

### `repositories/SqliteCategoryRepository.h/.cpp`

实现 `ICategoryRepository`，负责分类的 UPSERT、按 ID 查询和稳定排序的全部查询。
它使用显式编码保存收支类型：Income 为 0，Expense 为 1，不依赖 C++ 枚举底层
数值。读取数据库后仍通过 `Category::create()` 验证领域规则。

如果数据库出现 0 和 1 以外的类型编码，Repository 会记录错误并拒绝创建无效
Category。

### `repositories/SqliteTransactionRepository.h/.cpp`

实现 `ITransactionRepository`，负责账单的 UPSERT、按 ID 查询和按发生时间倒序查询：

- 金额以整数分保存，不使用浮点数。
- 收入和支出使用显式的 0、1 编码。
- 时间转为 UTC 后使用带毫秒的 ISO 8601 文本保存。
- 空备注保存为数据库空文本，而不是 SQL `NULL`。
- 账户与分类 UUID 由数据库外键进行最后一道一致性保护。
- 查询行重新通过 `Transaction::create()` 验证后才返回上层。

它不检查账户是否归档、分类是否归档或分类类型是否匹配；这些用户操作规则由
`CreateTransaction` 负责。Repository 和数据库只保证数据映射与引用完整性。

## 8. 测试支持代码

### `tests/unit/accounting/support/InMemoryAccountRepository.h`

这是 `IAccountRepository` 的内存测试实现：

- 使用 `std::vector<Account>` 保存账户。
- 新 UUID 会追加账户。
- 相同 UUID 会替换已有账户。
- 可以按 ID 查询或返回全部账户。
- 可以通过 `setSaveShouldSucceed(false)` 模拟存储失败。

该文件只能用于测试，不是正式存储方案。程序退出后数据会消失，因此不能把它
连接到最终用户界面代替 SQLite。

### `tests/unit/accounting/support/InMemoryCategoryRepository.h`

这是 `ICategoryRepository` 的内存测试实现。它支持新增、同 UUID 更新、按 ID
查询和返回全部分类，也能模拟保存失败，供 CreateCategory 的 Application 测试
使用。

### `tests/unit/accounting/support/InMemoryTransactionRepository.h`

实现 `ITransactionRepository` 的内存版本，支持新增、同 UUID 更新、查询和模拟保存
失败。它与账户、分类内存 Repository 一起用于隔离数据库测试 CreateTransaction。

## 9. 自动测试文件

目录：

```text
tests/unit/accounting/
```

每个测试 `.cpp` 都包含自己的 `QTEST_APPLESS_MAIN`，因此必须拥有独立的 CMake
可执行目标，不能把两个测试文件放进同一个 `add_executable`。

| 文件 | 验证内容 |
| --- | --- |
| `MoneyTest.cpp` | 金额创建、正负零判断、加减法和相等比较。 |
| `TransactionTest.cpp` | 有效账单、空 UUID、非正金额、无效时间和备注清理。 |
| `AccountTest.cpp` | 账户名称边界、期初负余额和归档状态。 |
| `CategoryTest.cpp` | 收入/支出分类、名称边界和归档状态。 |
| `AccountRepositoryTest.cpp` | 内存 Repository 的新增、查找、替换和全部查询契约。 |
| `CreateAccountTest.cpp` | 创建账户完整流程、无效输入、存储失败和 UUID 唯一性。 |
| `CategoryRepositoryTest.cpp` | 分类内存 Repository 的保存、查询和更新契约。 |
| `CreateCategoryTest.cpp` | 创建分类流程、类型保留、无效输入和存储失败。 |
| `TransactionRepositoryTest.cpp` | 账单内存 Repository 的保存、查询和更新契约。 |
| `CreateTransactionTest.cpp` | 跨实体校验、七类失败结果和成功保存流程。 |

测试代码不是产品功能，但它定义了当前代码必须持续满足的行为。修改业务代码后，
应运行全部测试，而不是只运行本次新增的测试。

### `tests/integration/accounting/SqliteDatabaseTest.cpp`

这是 SQLite 集成测试，验证：

- Qt 可以加载 `QSQLITE` 驱动。
- 首次迁移能创建 `accounts`、`categories`、`transactions` 表和预期字段。
- transactions 的三个查询索引已经创建。
- 重复迁移不会重复写入版本记录。
- 版本 1 和版本 2 数据库都能升级到版本 3，且旧数据保持不变。
- 每条 SQLite 连接都启用了外键约束。

它使用 `QTEST_GUILESS_MAIN` 创建 `QCoreApplication`，让 Qt 能加载 SQL 驱动；每个
测试都在构建工作目录创建独立临时数据库，并在结束后自动清理。

### `tests/integration/accounting/SqliteAccountRepositoryTest.cpp`

验证账户的保存、查询、更新、缺失结果和全部查询，并将 `CreateAccount` 与真实
SQLite Repository 组装起来验证跨层流程。测试先销毁 Repository，再关闭数据库，
避免 Qt 报告连接仍被使用。

### `tests/integration/accounting/SqliteCategoryRepositoryTest.cpp`

验证收入与支出分类的保存、查询、同 UUID 更新、稳定排序和缺失结果，并将
`CreateCategory` 与真实 SQLite Repository 组装起来验证完整调用链。

### `tests/integration/accounting/SqliteTransactionRepositoryTest.cpp`

验证账单所有字段的保存与恢复、整数分金额、UTC 时间文本、空备注、UPSERT、倒序
查询和缺失结果；同时验证无效外键写入失败、被账单引用的账户和分类不能删除，
以及 `CreateTransaction` 能通过三个真实 Repository 完成持久化。

## 10. CMake 文件如何组织 C++ 代码

### 根 `CMakeLists.txt`

负责整个工程：

- 查找 Qt 组件。
- 启用 CTest。
- 按依赖顺序加入 Domain、Application、Infrastructure 等子目录。
- 创建 StuKit 应用目标。
- 注册 QML 模块。

普通 C++ 业务文件不能加入 `qt_add_qml_module` 的 `QML_FILES`、`SOURCES` 或
`RESOURCES`；它们应由各层自己的静态库目标管理。

### `src/features/accounting/domain/CMakeLists.txt`

创建：

```text
StuKitAccountingDomain
```

它编译 Money、Transaction、Account 和 Category，并公开领域头文件目录。

### `src/features/accounting/application/CMakeLists.txt`

创建：

```text
StuKitAccountingApplication
```

它编译 CreateAccount，并链接 `StuKitAccountingDomain`。依赖不能反过来。

### `src/features/accounting/infrastructure/CMakeLists.txt`

创建 `StuKitAccountingInfrastructure`，编译 `SqliteDatabase` 和三个 SQLite
Repository，并链接 `Qt6::Sql` 和 `StuKitAccountingDomain`。

### `tests/CMakeLists.txt`

负责加入记账单元测试和数据库集成测试目录。

### `tests/unit/accounting/CMakeLists.txt`

为每个 Qt Test 类创建独立可执行程序，并通过 `add_test` 注册给 CTest。

### `tests/integration/accounting/CMakeLists.txt`

分别创建数据库迁移、账户、分类和账单 Repository 测试目标。三个 Repository
集成测试额外链接 Application，用于验证创建用例到 SQLite 的完整调用链。

## 11. 建议的代码阅读顺序

第一次阅读项目时，建议按照下面的顺序：

1. `Money.h` 和 `Money.cpp`：理解最小的值对象。
2. `Account.h` 和 `Account.cpp`：理解实体及工厂函数。
3. `IAccountRepository.h`：理解存储抽象。
4. `CreateAccount.h` 和 `CreateAccount.cpp`：理解用例如何编排 Domain。
5. `InMemoryAccountRepository.h`：理解接口的测试实现。
6. `CreateAccountTest.cpp`：观察完整流程如何被验证。
7. `SqliteDatabase.h/.cpp`：理解连接生命周期和数据库迁移。
8. `SqliteAccountRepository.h/.cpp`：理解实体与数据库行之间的转换。
9. `SqliteCategoryRepository.h/.cpp`：理解枚举的稳定数据库编码。
10. `SqliteTransactionRepository.h/.cpp`：理解外键、UTC 时间和账单字段映射。
11. SQLite 测试：理解迁移升级、真实数据库集成测试和资源析构顺序。
12. `main.cpp`：理解应用如何启动，但不要从入口文件寻找业务规则。

## 12. 修改代码时如何判断文件归属

可以依次提问：

- 这是单个业务对象必须始终满足的规则吗？放入 Domain 实体或值对象。
- 这是一次用户操作的流程吗？放入 Application 用例。
- 这是 SQL、文件或平台能力吗？放入 Infrastructure。
- 这是页面状态和 QML 数据转换吗？放入 Presentation/ViewModel。
- 这只是自动测试需要的替身吗？放入 `tests/`。

如果一个文件同时处理页面、SQL 和业务规则，通常说明职责需要继续拆分。

## 13. 后续更新规则

每完成一课并新增 C++ 文件，都应在本文补充它的职责、依赖和禁止承担的职责。
下一阶段将扩展账单 Repository 的时间范围查询，并实现按用户本地月份汇总收支的
Application 用例。
