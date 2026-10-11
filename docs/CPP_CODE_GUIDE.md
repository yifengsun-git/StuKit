# StuKit C++ 代码导读

## 1. 文档目的

本文说明 StuKit 中各个 C++ 头文件和源文件的职责、依赖关系以及不能承担的职责。
它适合在阅读代码、排查编译错误或添加新功能前快速查阅。

当前内容对应第二十八课完成后的代码，包括三版 SQLite 迁移、三个 Repository
接口、创建与查询用例、记账 Presentation，以及从 QML 创建账户、分类和账单并
持久化到真实 SQLite 的端到端链路、账户余额、月度流水筛选、安全备份恢复和
全部流水 CSV 导出。

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
  -> AccountingModule
  -> QML 应用外壳

AccountingModule
  -> UserSettings
  -> SqliteDatabase / SqliteBackupService / 三个 SQLite Repository
  -> 创建、查询与导出数据用例
  -> AccountingOverviewViewModel / AccountingSetupViewModel
  -> CreateTransactionViewModel / AccountBalancesViewModel

DataMaintenanceViewModel
  -> AccountingModule::backupDatabase / restoreDatabase / exportTransactions
  -> QML 文件选择与状态提示

CreateAccount
  -> IAccountRepository
  -> Account
       -> Money

CreateTransaction
  -> ITransactionRepository
  -> IAccountRepository
  -> ICategoryRepository
  -> Transaction

GetMonthlyOverview
  -> ITransactionRepository::findOccurredBetween
  -> IAccountRepository::findAll / ICategoryRepository::findAll
  -> 用户自定义 QTimeZone
  -> MonthlyOverview / MonthlyTransactionItem

GetTransactionExportData
  -> 三个 Repository::findAll
  -> TransactionExportData / TransactionExportItem

CsvTransactionWriter
  -> TransactionExportData / 用户自定义 QTimeZone
  -> UTF-8 CSV / QSaveFile

GetAccountingSetup
  -> IAccountRepository::findAll
  -> ICategoryRepository::findAll
  -> AccountingSetup

GetAccountBalances
  -> IAccountRepository::findAll
  -> ITransactionRepository::findAll
  -> AccountBalances / AccountBalanceItem

AccountingOverviewViewModel
  -> GetMonthlyOverview
  -> TransactionListModel
  -> TransactionFilterProxyModel
  -> Q_PROPERTY / signals / Q_INVOKABLE

AccountingSetupViewModel
  -> GetAccountingSetup / CreateAccount / CreateCategory
  -> UpdateAccount / UpdateCategory
  -> AccountListModel / CategoryListModel
  -> MoneyInputParser

CreateTransactionViewModel
  -> GetAccountingSetup / GetTransaction
  -> CreateTransaction / UpdateTransaction / DeleteTransaction
  -> AccountListModel / CategoryListModel
  -> 用户自定义 QTimeZone / MoneyInputParser

AccountBalancesViewModel
  -> GetAccountBalances
  -> AccountBalanceListModel
  -> MoneyFormatter

InMemoryAccountRepository（仅测试）
  -> IAccountRepository

SqliteDatabase
  -> Qt SQL / QSQLITE
  -> schema_migrations 表
  -> accounts 表
  -> categories 表
  -> transactions 表与索引

SqliteBackupService
  -> SQLite VACUUM INTO / PRAGMA quick_check
  -> QSaveFile 原子替换

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
Infrastructure --> Application -> Domain
Infrastructure ------------------> Domain
```

记账 QML 已通过根属性连接五个 ViewModel：`AccountingOverviewViewModel` 读取月度
汇总和流水，`AccountingSetupViewModel` 读取并创建账户与分类，
`CreateTransactionViewModel` 收集并提交新增或编辑账单表单并编排删除操作，
`AccountBalancesViewModel` 提供账户当前余额与活动账户总资产，
`DataMaintenanceViewModel` 编排备份、恢复、CSV 导出和界面重绑定。全新数据库会显示首次设置引导，
不会自动写入演示数据。账户与分类创建入口保持常驻，首次设置完成后仍可继续添加。

## 4. 程序入口

### `src/app/main.cpp`

作用：

- 创建 `QGuiApplication`，启动 Qt GUI 事件循环。
- 设置组织名和应用名，供设置、路径等 Qt API 使用。
- 创建 `QQmlApplicationEngine`。
- 使用 `QStandardPaths` 获取正式应用数据目录。
- 创建 `QSettings`、`AccountingModule` 和 `DataMaintenanceViewModel`。
- 通过根初始属性把 ViewModel 与启动错误传给 QML。
- 恢复数据库后把重建的记账 ViewModel 重新设置到 QML 根属性。
- 从 QML 模块 `StuKit` 加载 `Main.qml`。
- 在 QML 根对象创建失败时，以错误码退出。

它是整个应用的启动入口，但不应该包含：

- 记账规则。
- SQL 查询。
- 页面布局。
- 金额计算。

对象的具体创建集中在 `AccountingModule`；`main.cpp` 只安排应用启动，不包含业务
规则或 SQL。`STUKIT_APP_DATA_DIR` 只用于开发测试显式覆盖数据目录，默认仍使用
跨平台标准路径。

### `src/app/UserSettings.h/.cpp`

封装记账自定义时区设置。它通过 `QSettings` 保存 Qt 时区 ID，既能表示
`Asia/Shanghai`，也能表示 `UTC+08:00`。缺失或无效值回退到固定 UTC+08:00，
不会读取设备系统时区。

### `src/app/AccountingModule.h/.cpp`

这是记账模块的组合根，负责：

1. 创建应用数据目录并打开 `stukit.sqlite`。
2. 执行数据库迁移。
3. 创建备份服务和三个 SQLite Repository。
4. 创建账户、分类、账单 CRUD 与月度查询用例。
5. 创建四个记账 ViewModel 并连接基础数据、时区和账单变更信号。
6. 加载初始数据并保存后续时区变化。
7. 编排数据库备份、恢复前安全快照、连接释放、替换、重新初始化和失败回退。
8. 组合导出查询与 CSV 写入器，使用当前用户记账时区导出全部流水。

成员按数据库、Repository、用例、ViewModel 顺序声明，并反向销毁，确保数据库连接
最后关闭。其他层不能依赖这个类。

### `src/app/DataMaintenanceViewModel.h/.cpp`

这是数据维护操作与 QML 之间的桥梁：

- 将 `FileDialog` 返回的本地 `QUrl` 转换为路径。
- 调用 `AccountingModule` 执行备份、恢复或 CSV 导出。
- 提供忙碌、成功和错误状态。
- 恢复尝试后通知 `main.cpp` 重新绑定组合根中的记账 ViewModel。

它不直接运行 SQL 或复制文件。以后 Android 返回内容 URI 时，应在平台适配边界先
取得可读写的本地临时文件，再复用相同的数据库维护流程。

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
findAll()       返回全部账户，或用 nullopt 表示查询失败
```

这是纯抽象接口，不能直接创建对象。它描述 Application 需要的存储能力，但不说明
数据存在哪里。`findAll()` 的有值空 vector 表示查询成功但没有账户，`nullopt` 表示
存储失败或数据库数据无效，这两个状态不能混用。

该文件不能包含 `QSqlDatabase` 或 SQL 语句。SQLite Repository 将在
Infrastructure 层实现这个接口。

### `ICategoryRepository.h`

声明分类存储契约，提供保存、按 UUID 查找和返回全部分类的能力。全部查询与账户
接口采用相同的 `optional<vector<...>>` 成功/失败约定。它与
`IAccountRepository` 分开，使两个业务概念以后可以独立增加查询方法。

接口只依赖 Category 和 Qt Core 基础类型，不依赖 SQLite。单元测试使用内存实现，
正式持久化由 Infrastructure 中的 `SqliteCategoryRepository` 提供。

### `ITransactionRepository.h`

声明账单的存储契约：保存或更新、按 UUID 查询、删除、返回全部账单，以及查询一个左闭
右开时间范围内的账单。`findAll()` 与 `findOccurredBetween()` 都使用 optional 包裹
vector：空 vector 表示查询成功但没有流水，`std::nullopt` 表示存储失败或数据无效。
`removeById()` 使用 `RemoveTransactionResult` 区分删除成功、记录不存在和存储失败。

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

### `UpdateAccount.h/.cpp` 与 `UpdateCategory.h/.cpp`

实现账户与分类的补丁式更新：请求必须包含有效 UUID，并至少提供新名称或新归档
状态。用例先通过 Repository 读取原实体，再由领域工厂重新验证并保存：

- 更新账户始终保留原 UUID 和期初余额。
- 更新分类始终保留原 UUID 和收支类型。
- 未提供的字段沿用原值，避免两个独立界面操作互相覆盖。
- 结果区分无效输入、不存在和存储失败。

这两个用例不允许修改期初余额或分类类型；将来若支持这些高影响操作，应建立单独
用例并明确历史数据语义。

### `CreateTransaction.h` 与 `CreateTransaction.cpp`

实现新增账单的跨实体流程：

1. 调用 `Transaction::create()` 验证金额、UUID、时间和备注。
2. 查询账户并拒绝不存在或已归档账户。
3. 查询分类并拒绝不存在或已归档分类。
4. 检查分类类型与账单收入/支出类型一致。
5. 通过 `ITransactionRepository` 保存有效账单。

结果错误枚举能区分每种失败原因，后续 Presentation 可以把稳定错误类型转换为
用户提示，而不用解析字符串。

### `GetTransaction.h/.cpp`、`UpdateTransaction.h/.cpp` 与 `DeleteTransaction.h/.cpp`

- `GetTransaction` 为编辑表单读取指定 UUID 的原账单。
- `UpdateTransaction` 先检查原账单存在，再重做实体、账户、分类和收支类型校验，
  最后以原 UUID 保存，避免错误 UPSERT 出新账单。
- `DeleteTransaction` 验证 UUID，并把 Repository 的三种删除结果转换为稳定用例结果。

### `GetMonthlyOverview.h` 与 `GetMonthlyOverview.cpp`

实现指定年月的流水查询、关联名称组装和收支汇总：

1. 接收年份、月份和用户自定义 `QTimeZone`。
2. 使用自定义时区计算当月和下月第一天的起始时刻。
3. 转换为 UTC 后查询左闭右开范围。
4. 各读取一次全部账户和分类，建立 UUID 到名称的内存索引。
5. 组装保留原 `Transaction` 的 `MonthlyTransactionItem`，不为每条流水单独查询。
6. 分别累加收入与支出，并计算收入减支出的结余。
7. 区分无效请求、存储失败、缺失关联和成功的空月份。

用例不读取设备系统时区。固定偏移或地区时区由调用者显式传入，因此 Windows 和
Android 可以使用同一用户设置。

### `GetTransactionExportData.h/.cpp`

为跨软件导出准备稳定的数据快照：

1. 分别读取全部流水、账户和分类。
2. 建立账户、分类 UUID 到名称的内存索引，包括已归档项。
3. 按发生时间倒序、UUID 升序建立稳定顺序。
4. 将原始 `Transaction` 与可读名称组装为 `TransactionExportItem`。
5. 任一查询失败或关联缺失时返回 `StorageFailure`，不产生部分导出数据。

该用例不知道 CSV 列、文本编码或目标文件路径，因此将来可以复用于 JSON 导出等
其他表现形式。

### `GetAccountingSetup.h` 与 `GetAccountingSetup.cpp`

统一读取账户和分类，并返回 `AccountingSetup`。任意一次 Repository 查询失败时
返回 `StorageFailure`；两个查询都成功时，即使列表为空也返回成功。该用例让
Presentation 不需要分别协调两次查询，也不会把首次使用误报成数据库故障。

### `GetAccountBalances.h` 与 `GetAccountBalances.cpp`

一次读取全部账户和全部流水，按账户 UUID 推导每个账户的当前余额：

```text
当前余额 = 期初余额 + 全部收入 - 全部支出
```

结果中的 `AccountBalanceItem` 同时携带 Account 与当前余额，`AccountBalances` 还提供
全部未归档账户的总资产。归档账户保留并显示自己的余额，但不计入活动总资产。
用例使用整数分并在每次账户累加和总资产累加前检查 `qint64` 上下界；发现缺失账户
关联、Repository 查询失败或金额溢出时返回稳定错误，不向 Presentation 提供部分结果。

## 7. Infrastructure：基础设施层

目录：

```text
src/features/accounting/infrastructure/
```

Infrastructure 负责数据库、文件和平台 API 等外部技术细节。它可以依赖 Domain，
也可以消费 Application 定义的输出 DTO，但 Domain 和 Application 不能反向依赖
Infrastructure。

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

### `backup/SqliteBackupService.h/.cpp`

封装 SQLite 与文件系统相关的备份恢复原语：

- 使用 `VACUUM INTO` 从活动连接创建一致快照。
- 验证 SQLite 完整性、连续迁移历史、关键表和受支持版本。
- 拒绝活动数据库与目标文件为同一路径。
- 先验证临时快照，再通过 `QSaveFile` 原子提交。
- 只在数据库连接已经由组合根释放后替换正式数据库文件。

它不知道页面、当前月份或确认对话框，也不负责重建 Repository 和 ViewModel；这些
跨对象生命周期操作由 `AccountingModule` 编排。

### `export/CsvTransactionWriter.h/.cpp`

将 `TransactionExportData` 写成稳定的 UTF-8 CSV：

- 第一行输出固定中文列名，并添加 UTF-8 BOM 方便 Windows 表格软件识别中文。
- 金额直接从整数分格式化为两位小数，不经过浮点数。
- 同时输出带毫秒 UTC 时间、用户记账时区时间和时区 ID。
- 每个字段统一加双引号，字段中的双引号重复一次，安全容纳逗号和换行。
- 使用 CRLF 行尾并通过 `QSaveFile` 原子提交。
- 默认拒绝覆盖已有文件，只有调用者明确要求时才替换。

写入器不查询数据库，也不决定导出哪些流水。它消费已经完成关联的 Application DTO，
因此文件格式逻辑可以独立测试。

### `repositories/SqliteAccountRepository.h`

声明 `IAccountRepository` 的 SQLite 实现：

- `save()` 保存或更新账户。
- `findById()` 按 UUID 查询账户。
- `findAll()` 返回稳定排序的全部账户，并区分成功空列表与查询失败。
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

账户与分类 SQLite Repository 的 `findAll()` 在连接未打开、SQL 执行失败或某行无法
恢复为有效实体时返回 `nullopt`。只有查询真正成功且没有行时才返回空 vector。

### `repositories/SqliteTransactionRepository.h/.cpp`

实现 `ITransactionRepository`，负责账单的 UPSERT、按 ID 查询、删除、全部查询和 UTC
时间范围查询：

- 金额以整数分保存，不使用浮点数。
- 收入和支出使用显式的 0、1 编码。
- 时间转为 UTC 后使用带毫秒的 ISO 8601 文本保存。
- 空备注保存为数据库空文本，而不是 SQL `NULL`。
- 账户与分类 UUID 由数据库外键进行最后一道一致性保护。
- 查询行重新通过 `Transaction::create()` 验证后才返回上层。
- 时间范围查询使用参数绑定、左闭右开条件和稳定倒序。
- 全部查询与范围查询都用 `nullopt` 表示连接、SQL 或数据恢复失败；成功空结果使用空 vector。

它不检查账户是否归档、分类是否归档或分类类型是否匹配；这些用户操作规则由
`CreateTransaction` 负责。Repository 和数据库只保证数据映射与引用完整性。

## 8. Presentation：C++ 表现层

目录：

```text
src/features/accounting/presentation/
```

Presentation 把 Application 结果转换为 QML 容易消费的属性、信号和模型。它可以
依赖 Application，但不执行 SQL，也不重新实现月度统计。

### `MoneyFormatter.h/.cpp`

把整数分格式化成固定两位小数文本。实现始终使用整数除法和取余，不把 Money
转换为 `double`，并正确处理 `qint64` 最小值。当前只输出数值，不提前决定人民币
符号、千位分隔或本地化策略。

### `MoneyInputParser.h/.cpp`

把 QML 金额文本逐字符转换为整数分，支持可选正负号和最多两位小数。解析过程不
使用浮点数，并对 `qint64` 最大值和最小值分别进行溢出检查。它只判断文本格式和
数值范围；账户允许负期初余额，而账单必须为正数的规则仍由领域实体判断。

### `AccountListModel.h/.cpp` 与 `CategoryListModel.h/.cpp`

把 Account、Category 暴露为 QML 可读取的列表角色。账户模型提供 UUID、名称、
期初余额文本和归档状态；分类模型提供 UUID、名称、收入标记和归档状态。模型不
查询数据库，数据只能由 ViewModel 整体替换。

### `TransactionListModel.h/.cpp`

继承 `QAbstractListModel`，把 `std::vector<MonthlyTransactionItem>` 暴露为 QML 列表角色：

```text
transactionId, isIncome, amountText, accountId,
categoryId, occurredAt, occurredAtText, note,
accountName, categoryName, typeText, fullOccurredAtText
```

`replaceTransactions()` 使用模型重置通知 QML 更新。`occurredAtText` 按 ViewModel
传入的自定义时区生成简短时间，`fullOccurredAtText` 生成详情用的完整时间，
防止 QML 使用设备时区。模型只转换单行展示数据，不查询
Repository，也不计算月度汇总。

### `TransactionFilterProxyModel.h/.cpp`

继承 `QSortFilterProxyModel`，在不复制源数据的情况下筛选 `TransactionListModel`：

- 类型条件支持全部、收入和支出。
- 账户与分类使用 UUID 精确匹配。
- 关键字对备注、账户名和分类名执行不区分大小写的包含匹配。
- 多个非空条件同时存在时取交集。

代理模型只决定哪些行对 QML 可见，不查询 Repository、不修改账单，也不重新计算
月收入、月支出或月结余。

### `AccountBalanceListModel.h/.cpp`

把 `std::vector<AccountBalanceItem>` 暴露为账户 ID、名称、期初余额文本、当前余额文本和
归档状态五个角色。金额格式化仍委托 `MoneyFormatter`，模型不读取 Repository，也不
重新累计流水。

### `AccountingOverviewViewModel.h/.cpp`

记账首页的页面状态桥梁，提供：

- 当前年月、月份标签和自定义时区 ID。
- 月收入、月支出、月结余文本。
- 空数据与错误状态。
- 流水 `QAbstractItemModel`。
- 流水类型、账户、分类、关键字筛选状态及匹配数量。
- 从完整月度流水生成的筛选选项，包括历史账单引用的归档项。
- 上一月、下一月、加载和切换时区操作。
- 地区时区及 -14:00 到 +14:00 固定偏移选择。

ViewModel 调用 `GetMonthlyOverview`，但不知道数据来自 SQLite 还是内存测试实现。
改变月份或时区会重新执行用例。筛选通过 `TransactionFilterProxyModel` 只影响列表，
不会改变完整月份汇总。错误枚举在这里转换为面向用户的稳定中文提示。

### `AccountBalancesViewModel.h/.cpp`

调用 `GetAccountBalances`，向 QML 提供账户余额模型、活动账户总资产文本和错误状态。
存储失败与金额溢出会转换为稳定中文提示；失败时会清空旧列表和旧总资产，避免界面
继续展示已经失效的数据。它只负责页面状态转换，不参与余额业务计算。

### `AccountingSetupViewModel.h/.cpp`

负责基础资料页面状态：账户与分类模型、是否存在未归档账户、是否存在收入/支出
分类，以及错误提示。它调用 `GetAccountingSetup` 加载数据，调用 CreateAccount、
CreateCategory 保存输入，并通过 UpdateAccount、UpdateCategory 完成重命名、归档
和恢复。每次成功操作后都重新查询真实 Repository 并发出 `setupChanged`，驱动
账单选项和首页刷新。它不直接执行 SQL，也不在 QML 中解析金额。

### `CreateTransactionViewModel.h/.cpp`

负责新增、编辑和删除账单的状态与编排：

- 只向账户模型放入未归档账户。
- 根据收入或支出类型过滤未归档分类。
- 维护金额、备注、所选 UUID、日期和时间文本。
- 复用 `MoneyInputParser`，并拒绝零金额和负金额。
- 使用用户自定义 `QTimeZone` 构造发生时刻。
- 使用 `QDateTime::TransitionResolution::Reject` 拒绝夏令时缺口和重复时间。
- 通过 `GetTransaction` 回填原账单，并在记账时区中还原日期和时间。
- 按模式调用 `CreateTransaction` 或 `UpdateTransaction`，并转换稳定错误。
- 调用 `DeleteTransaction`，成功时发出删除信号。
- 创建、更新和删除分别发出信号，失败时保留用户输入。

它不直接访问 SQLite。账户、分类和账单的最终业务校验仍由 Application 用例及
Domain 实体完成，QML 只绑定属性、选择模型行并触发 `submit()`。

## 9. 测试支持代码

### `tests/unit/accounting/support/InMemoryAccountRepository.h`

这是 `IAccountRepository` 的内存测试实现：

- 使用 `std::vector<Account>` 保存账户。
- 新 UUID 会追加账户。
- 相同 UUID 会替换已有账户。
- 可以按 ID 查询或返回全部账户。
- 可以通过 `setSaveShouldSucceed(false)` 模拟存储失败。
- 可以通过 `setFindAllShouldSucceed(false)` 单独模拟全部查询失败。

该文件只能用于测试，不是正式存储方案。程序退出后数据会消失，因此不能把它
连接到最终用户界面代替 SQLite。

### `tests/unit/accounting/support/InMemoryCategoryRepository.h`

这是 `ICategoryRepository` 的内存测试实现。它支持新增、同 UUID 更新、按 ID
查询和返回全部分类，也能分别模拟保存失败和全部查询失败，供 Application 与
Presentation 测试使用。

### `tests/unit/accounting/support/InMemoryTransactionRepository.h`

实现 `ITransactionRepository` 的内存版本，支持新增、同 UUID 更新、删除、全部查询和
时间范围查询，并能分别模拟保存、全部查询和范围查询失败。它与账户、分类内存 Repository 一起用于隔离
数据库测试 Application 用例。

## 10. 自动测试文件

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
| `TransactionRepositoryTest.cpp` | 账单保存、更新及左闭右开时间范围查询契约。 |
| `CreateTransactionTest.cpp` | 跨实体校验、七类失败结果和成功保存流程。 |
| `ManageTransactionTest.cpp` | 账单读取、保留 UUID 更新、防误新建、删除与错误区分。 |
| `ManageSetupTest.cpp` | 账户/分类补丁更新、字段保留、不存在及存储失败。 |
| `GetMonthlyOverviewTest.cpp` | 时区边界、跨年、汇总、关联名称、归档项、查询次数和错误。 |
| `GetAccountBalancesTest.cpp` | 期初余额、收支累计、归档口径、查询失败、缺失关联和整数溢出。 |
| `GetTransactionExportDataTest.cpp` | 全量关联、已归档名称、稳定排序、查询失败和缺失关联。 |
| `AccountingPresentationTest.cpp` | 金额、可读名称、完整/简短时间、页面导航、筛选状态及汇总不变性。 |
| `AccountBalancesPresentationTest.cpp` | 余额角色、总资产格式、错误映射及失败后清理旧状态。 |
| `TransactionFilterProxyModelTest.cpp` | 类型、账户、分类、关键字、组合条件、清空和源模型重置。 |
| `AccountingSetupTest.cpp` | 金额输入边界、基础数据查询、创建、重命名、归档、恢复和错误映射。 |
| `CreateTransactionPresentationTest.cpp` | 选项过滤、时区输入、编辑回填、更新/删除信号与错误保留。 |

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

### `tests/integration/accounting/SqliteBackupServiceTest.cpp`

验证一致快照包含真实数据、已有目标默认受保护、明确覆盖后可原子替换、损坏文件
会被拒绝、未来迁移版本会被拒绝，以及关闭连接后能用有效备份替换数据库。

### `tests/integration/accounting/CsvTransactionWriterTest.cpp`

验证 UTF-8 BOM、固定表头、自定义时区、精确金额、中文、逗号、引号和多行备注转义；
还验证空数据、已有目标保护、明确覆盖和无效输入。测试读取真实临时文件，但不需要
启动图形界面。

### `tests/integration/accounting/SqliteCategoryRepositoryTest.cpp`

验证收入与支出分类的保存、查询、同 UUID 更新、稳定排序和缺失结果，并将
`CreateCategory` 与真实 SQLite Repository 组装起来验证完整调用链。

### `tests/integration/accounting/SqliteTransactionRepositoryTest.cpp`

验证账单所有字段的保存与恢复、整数分金额、UTC 时间文本、空备注、UPSERT、倒序
查询、删除和缺失结果；同时验证时间范围的左闭右开边界、无效外键写入失败、被账单引用
的账户和分类不能删除，以及 `CreateTransaction` 能通过三个真实 Repository 完成
持久化。

### `tests/integration/app/AccountingModuleTest.cpp`

使用临时目录和真实 SQLite 验证应用组合根：数据库创建并迁移到版本 3、空月份
ViewModel 可用、自定义固定时区可持久化、无效设置回退到 UTC+08:00，并拒绝空的
应用数据目录。它还通过组合根创建账户和两类分类，销毁并重建模块后确认 SQLite
数据仍然存在，并验证账户与分类重命名、归档、恢复可跨重启持久化，同时确认空库
不会自动产生默认记录。测试还通过表单 ViewModel 写入真实账单，验证创建、编辑和
删除后月度首页立即刷新，归档项的历史关联名称仍可读，以及每次重启后 SQLite
保持最新状态；并验证余额随期初值及账单新增、编辑、删除同步变化，重启后仍可从
真实 SQLite 流水重新推导，归档账户不会计入活动总资产。它还验证真实数据库可以
备份、修改后恢复，恢复前安全快照存在，旧 ViewModel 被销毁并重新创建，以及无效
候选文件不会触碰当前数据；同时验证 QML 使用的数据维护 ViewModel 能通过组合根把
真实 SQLite 流水导出为 CSV。

## 11. CMake 文件如何组织 C++ 代码

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

它编译账户、分类和账单的创建/更新用例，以及 GetMonthlyOverview、
GetAccountingSetup、GetAccountBalances 和 GetTransactionExportData，并链接
`StuKitAccountingDomain`。依赖不能反过来。

### `src/features/accounting/infrastructure/CMakeLists.txt`

创建 `StuKitAccountingInfrastructure`，编译 `SqliteDatabase`、`SqliteBackupService`、
`CsvTransactionWriter` 和三个 SQLite Repository，并链接 `Qt6::Sql`、
`StuKitAccountingApplication` 和 `StuKitAccountingDomain`。

### `src/features/accounting/presentation/CMakeLists.txt`

创建 `StuKitAccountingPresentation`，编译金额格式化与解析、四个列表模型、一个
筛选代理模型和四个 ViewModel，并链接 `Qt6::Core` 与
`StuKitAccountingApplication`。它不链接 Qt SQL。

### `src/app/CMakeLists.txt`

创建 `StuKitAppCore`，编译组合根、用户设置和数据维护 ViewModel。只有这个最外层目标同时链接
Infrastructure 与 Presentation，负责把两条依赖链组装起来。

### `tests/CMakeLists.txt`

负责加入记账单元测试和数据库集成测试目录。

### `tests/unit/accounting/CMakeLists.txt`

为每个 Qt Test 类创建独立可执行程序，并通过 `add_test` 注册给 CTest。

### `tests/integration/accounting/CMakeLists.txt`

分别创建数据库迁移、备份服务、CSV 写入器、账户、分类和账单 Repository 测试目标。
三个 Repository 集成测试额外链接 Application，用于验证创建用例到 SQLite 的完整调用链。

## 12. 建议的代码阅读顺序

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
11. `GetAccountingSetup.h/.cpp`：理解成功空列表与存储失败如何分开。
12. `MoneyInputParser.h/.cpp`：理解金额文本如何精确转换为整数分。
13. `AccountListModel` 与 `CategoryListModel`：理解实体如何成为 QML 列表角色。
14. `AccountingSetupViewModel.h/.cpp`：理解基础资料创建、重命名与归档流程。
15. `TransactionListModel.h/.cpp`：理解流水如何成为 QML 列表角色。
16. `TransactionFilterProxyModel.h/.cpp`：理解代理模型如何筛选源模型而不复制数据。
17. `AccountingOverviewViewModel.h/.cpp`：理解月份、时区、汇总与筛选状态。
18. `GetAccountBalances.h/.cpp`：理解余额如何由期初值与全部流水安全推导。
19. `AccountBalanceListModel.h/.cpp`：理解余额结果如何成为 QML 列表角色。
20. `AccountBalancesViewModel.h/.cpp`：理解总资产文本、刷新和错误状态。
21. `CreateTransactionViewModel.h/.cpp`：理解表单输入、过滤和提交编排。
22. `CreateTransactionPresentationTest.cpp`：理解表单状态如何脱离 QML 测试。
23. `AccountingSetupTest.cpp`：理解基础数据输入、查询、模型和错误映射。
24. `UserSettings.h/.cpp`：理解自定义时区如何跨启动保存。
25. `SqliteBackupService.h/.cpp`：理解一致快照、验证和原子文件替换。
26. `GetTransactionExportData.h/.cpp`：理解全量查询、名称关联和稳定排序。
27. `CsvTransactionWriter.h/.cpp`：理解 CSV 转义、编码、时区与原子写入。
28. `AccountingModule.h/.cpp`：理解信号连接、依赖组装、恢复回退和资源析构顺序。
29. `DataMaintenanceViewModel.h/.cpp`：理解文件 URL、操作状态和 QML 对象重绑定通知。
30. `AccountingHomePage.qml`：理解 QML 如何只绑定状态、选择文件和发送操作。
31. `main.cpp`：理解标准路径、根属性注入、恢复后重绑定和应用启动。

## 13. 修改代码时如何判断文件归属

可以依次提问：

- 这是单个业务对象必须始终满足的规则吗？放入 Domain 实体或值对象。
- 这是一次用户操作的流程吗？放入 Application 用例。
- 这是 SQL、文件或平台能力吗？放入 Infrastructure。
- 这是页面状态和 QML 数据转换吗？放入 Presentation/ViewModel。
- 这只是自动测试需要的替身吗？放入 `tests/`。

如果一个文件同时处理页面、SQL 和业务规则，通常说明职责需要继续拆分。

## 14. 后续更新规则

每完成一课并新增 C++ 文件，都应在本文补充它的职责、依赖和禁止承担的职责。
下一阶段将建立 Windows Release 部署流程。应用安装规则和部署命令属于构建边界，
不应进入 Domain、Application 或 Repository；部署目录属于生成产物，不提交 Git。
