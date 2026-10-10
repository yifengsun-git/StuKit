# 第十二课：实现 SQLite Account Repository

## 本课目标

本课实现 `IAccountRepository` 的 SQLite 版本，让账户不再只保存在测试内存中，
而是能够写入第十一课创建的 `accounts` 表。

完成后，你应该理解：

- Repository 接口和 Repository 实现之间的关系。
- 如何使用参数绑定执行 SQL，避免拼接用户输入。
- 如何把 `Account` 转换成数据库字段，再从查询结果恢复为 `Account`。
- 如何使用 SQLite UPSERT 同时支持新增和更新。
- 为什么数据库实现属于 Infrastructure，而接口仍属于 Domain。
- 如何让 `CreateAccount` 用例在不修改自身代码的情况下改用 SQLite。
- 如何管理数据库、Repository 和临时目录的析构顺序。

本课不制作账户页面，也不在 `main.cpp` 中组装正式数据库。界面接入会在后续
Presentation 课程中进行。

## 1. 本课要连接的两端

现在项目里已经存在：

```text
Domain
  IAccountRepository
    save(account)
    findById(id)
    findAll()

Infrastructure
  SqliteDatabase
    open(path)
    migrate()
    connection()
```

但两者之间还没有实现类。本课新增：

```text
CreateAccount
      |
      v
IAccountRepository              Domain 中的抽象接口
      ^
      |
SqliteAccountRepository         Infrastructure 中的具体实现
      |
      v
QSqlDatabase -> accounts 表
```

依赖方向仍然正确：Infrastructure 可以依赖 Domain，Domain 不知道 SQLite 的
存在。`CreateAccount` 也只依赖接口，因此不需要为了更换存储方式而修改。

## 2. 本课文件变化

新增：

```text
src/features/accounting/infrastructure/repositories/SqliteAccountRepository.h
src/features/accounting/infrastructure/repositories/SqliteAccountRepository.cpp
tests/integration/accounting/SqliteAccountRepositoryTest.cpp
```

修改：

```text
src/features/accounting/application/CreateAccount.h
src/features/accounting/infrastructure/CMakeLists.txt
tests/integration/accounting/CMakeLists.txt
```

不要把这些 C++ 文件加入根目录的 `qt_add_qml_module()`。它们属于普通静态库和
测试目标，不是 QML 资源。

## 3. 先修正 CreateAccount.h 的直接依赖

打开 `src/features/accounting/application/CreateAccount.h`，在标准库 include
区域加入：

```cpp
#include <variant>
```

这个头文件直接使用了 `std::variant`，所以必须自己包含 `<variant>`。之前能够
构建只是因为其他头文件间接包含了它；依赖间接 include 会让代码在更换编译器或
调整头文件后突然失败。

完成后 include 区域应类似：

```cpp
#include <QString>

#include <variant>
```

## 4. 创建 Repository 头文件

创建目录：

```text
src/features/accounting/infrastructure/repositories/
```

创建 `SqliteAccountRepository.h`：

```cpp
#pragma once

#include "IAccountRepository.h"

#include <QSqlDatabase>
#include <QString>

namespace stukit::accounting {

class SqliteAccountRepository final : public IAccountRepository
{
public:
    explicit SqliteAccountRepository(QSqlDatabase database);

    [[nodiscard]] bool save(const Account &account) override;
    [[nodiscard]] std::optional<Account> findById(const QUuid &id) const override;
    [[nodiscard]] std::vector<Account> findAll() const override;

    [[nodiscard]] const QString &lastError() const noexcept;

private:
    [[nodiscard]] std::optional<Account> accountFromCurrentRow(
        const QSqlQuery &query) const;

    QSqlDatabase m_database;
    mutable QString m_lastError;
};

} // namespace stukit::accounting
```

由于私有函数使用了 `QSqlQuery`，还需要在 `namespace` 之前前置声明：

```cpp
class QSqlQuery;
```

最终位置应是：

```cpp
#include <QSqlDatabase>
#include <QString>

class QSqlQuery;

namespace stukit::accounting {
```

这里保存一份 `QSqlDatabase` 句柄是安全的：它是 Qt 对命名数据库连接的轻量句柄，
不是复制一份数据库文件。

`m_lastError` 使用 `mutable`，因为 `findById()` 和 `findAll()` 按 Repository 接口
要求是 `const`，但查询失败时仍需要记录诊断信息。它只保存技术错误，不改变账户
业务状态。

## 5. 实现构造函数和错误读取

创建 `SqliteAccountRepository.cpp`，先写基础结构：

```cpp
#include "repositories/SqliteAccountRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <utility>

namespace stukit::accounting {

SqliteAccountRepository::SqliteAccountRepository(QSqlDatabase database)
    : m_database(std::move(database))
{
}

const QString &SqliteAccountRepository::lastError() const noexcept
{
    return m_lastError;
}

} // namespace stukit::accounting
```

Repository 不负责打开数据库或执行迁移。调用顺序必须是：

```text
SqliteDatabase::open()
  -> SqliteDatabase::migrate()
  -> 创建 SqliteAccountRepository
```

这样 Repository 可以假定连接已经打开、`accounts` 表已经存在。

## 6. 实现 save：使用参数绑定和 UPSERT

在 `.cpp` 中实现：

```cpp
bool SqliteAccountRepository::save(const Account &account)
{
    m_lastError.clear();

    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO accounts("
        "id, name, opening_balance_cents, archived) "
        "VALUES(:id, :name, :openingBalanceCents, :archived) "
        "ON CONFLICT(id) DO UPDATE SET "
        "name = excluded.name, "
        "opening_balance_cents = excluded.opening_balance_cents, "
        "archived = excluded.archived"));

    query.bindValue(
        QStringLiteral(":id"),
        account.id().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":name"), account.name());
    query.bindValue(
        QStringLiteral(":openingBalanceCents"),
        account.openingBalance().cents());
    query.bindValue(
        QStringLiteral(":archived"),
        account.isArchived() ? 1 : 0);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}
```

不要写成：

```cpp
"INSERT ... VALUES ('" + account.name() + "')"
```

字符串拼接会产生引号转义问题，也会带来 SQL 注入风险。`prepare()` 和
`bindValue()` 会把 SQL 结构与数据分开。

`ON CONFLICT(id) DO UPDATE` 称为 UPSERT：ID 不存在时插入，ID 已存在时更新。
这与 `IAccountRepository::save()` 的契约一致。

## 7. 把查询结果恢复为 Account

在 `.cpp` 中实现私有转换函数：

```cpp
std::optional<Account> SqliteAccountRepository::accountFromCurrentRow(
    const QSqlQuery &query) const
{
    const auto account = Account::create(AccountData{
        .id = QUuid(query.value(QStringLiteral("id")).toString()),
        .name = query.value(QStringLiteral("name")).toString(),
        .openingBalance = Money::fromCents(
            query.value(QStringLiteral("opening_balance_cents")).toLongLong()),
        .archived = query.value(QStringLiteral("archived")).toInt() != 0,
    });

    if (!account.has_value()) {
        m_lastError = QStringLiteral("Database contains an invalid account");
    }

    return account;
}
```

不要绕过 `Account::create()` 直接构造对象。数据库内容也必须重新经过 Domain
规则验证，这可以防止损坏或旧版本数据生成无效实体。

## 8. 实现 findById

继续在 `.cpp` 中加入：

```cpp
std::optional<Account> SqliteAccountRepository::findById(
    const QUuid &id) const
{
    m_lastError.clear();

    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, name, opening_balance_cents, archived "
        "FROM accounts WHERE id = :id"));
    query.bindValue(
        QStringLiteral(":id"),
        id.toString(QUuid::WithoutBraces));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return accountFromCurrentRow(query);
}
```

“没有找到”不是数据库错误，因此返回 `std::nullopt` 时保持 `lastError()` 为空。
如果 SQL 执行失败，同样返回 `std::nullopt`，但 `lastError()` 会保存错误信息。

## 9. 实现 findAll

继续加入：

```cpp
std::vector<Account> SqliteAccountRepository::findAll() const
{
    m_lastError.clear();
    std::vector<Account> accounts;

    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return accounts;
    }

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT id, name, opening_balance_cents, archived "
            "FROM accounts ORDER BY name COLLATE NOCASE, id"))) {
        m_lastError = query.lastError().text();
        return accounts;
    }

    while (query.next()) {
        auto account = accountFromCurrentRow(query);
        if (!account.has_value()) {
            return {};
        }

        accounts.push_back(std::move(*account));
    }

    return accounts;
}
```

SQL 明确指定排序，避免依赖 SQLite 未保证的默认行顺序。这里按名称排序，相同名称
再按 ID 排序，使结果稳定。

## 10. 将 Repository 加入 Infrastructure 目标

修改 `src/features/accounting/infrastructure/CMakeLists.txt`：

```cmake
add_library(StuKitAccountingInfrastructure STATIC
    database/SqliteDatabase.cpp
    database/SqliteDatabase.h
    repositories/SqliteAccountRepository.cpp
    repositories/SqliteAccountRepository.h
)
```

其他部分保持不变。Infrastructure 已经公开链接 `Qt6::Sql` 和
`StuKitAccountingDomain`，不需要重复增加依赖。

然后在 Qt Creator 中执行一次“运行 CMake”，让新文件进入构建系统。

## 11. 创建 Repository 集成测试

创建 `tests/integration/accounting/SqliteAccountRepositoryTest.cpp`：

```cpp
#include "CreateAccount.h"
#include "database/SqliteDatabase.h"
#include "repositories/SqliteAccountRepository.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

#include <memory>
#include <variant>

using namespace stukit::accounting;

namespace {

QString uniqueConnectionName()
{
    return QStringLiteral("stukit-account-repository-test-%1").arg(
        QUuid::createUuid().toString(QUuid::WithoutBraces));
}

Account makeAccount(
    const QUuid &id,
    const QString &name,
    qint64 openingBalanceCents = 0,
    bool archived = false)
{
    const auto account = Account::create(AccountData{
        .id = id,
        .name = name,
        .openingBalance = Money::fromCents(openingBalanceCents),
        .archived = archived,
    });

    Q_ASSERT(account.has_value());
    return *account;
}

} // namespace

class SqliteAccountRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void savesAndFindsAccount();
    void returnsNulloptForMissingAccount();
    void updatesAccountWithSameId();
    void returnsAllAccounts();
    void worksWithCreateAccountUseCase();

private:
    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<SqliteDatabase> m_database;
    std::unique_ptr<SqliteAccountRepository> m_repository;
};

void SqliteAccountRepositoryTest::init()
{
    m_directory = std::make_unique<QTemporaryDir>(
        QDir::current().filePath(
            QStringLiteral("stukit-account-repository-test-XXXXXX")));
    QVERIFY(m_directory->isValid());

    m_database = std::make_unique<SqliteDatabase>(uniqueConnectionName());
    QVERIFY2(
        m_database->open(
            m_directory->filePath(QStringLiteral("stukit.sqlite"))),
        qPrintable(m_database->lastError()));
    QVERIFY2(m_database->migrate(), qPrintable(m_database->lastError()));

    m_repository = std::make_unique<SqliteAccountRepository>(
        m_database->connection());
}

void SqliteAccountRepositoryTest::cleanup()
{
    m_repository.reset();
    m_database.reset();
    m_directory.reset();
}

void SqliteAccountRepositoryTest::savesAndFindsAccount()
{
    const auto id = QUuid::createUuid();
    QVERIFY(m_repository->save(
        makeAccount(id, QStringLiteral("现金"), -1234, true)));

    const auto stored = m_repository->findById(id);
    QVERIFY2(stored.has_value(), qPrintable(m_repository->lastError()));
    QCOMPARE(stored->id(), id);
    QCOMPARE(stored->name(), QStringLiteral("现金"));
    QCOMPARE(stored->openingBalance().cents(), qint64{-1234});
    QVERIFY(stored->isArchived());
}

void SqliteAccountRepositoryTest::returnsNulloptForMissingAccount()
{
    const auto stored = m_repository->findById(QUuid::createUuid());

    QVERIFY(!stored.has_value());
    QVERIFY(m_repository->lastError().isEmpty());
}

void SqliteAccountRepositoryTest::updatesAccountWithSameId()
{
    const auto id = QUuid::createUuid();
    QVERIFY(m_repository->save(makeAccount(id, QStringLiteral("现金"))));
    QVERIFY(m_repository->save(
        makeAccount(id, QStringLiteral("日常账户"), 5000)));

    const auto accounts = m_repository->findAll();
    QVERIFY2(m_repository->lastError().isEmpty(),
             qPrintable(m_repository->lastError()));
    QCOMPARE(accounts.size(), std::size_t{1});
    QCOMPARE(accounts.front().name(), QStringLiteral("日常账户"));
    QCOMPARE(accounts.front().openingBalance().cents(), qint64{5000});
}

void SqliteAccountRepositoryTest::returnsAllAccounts()
{
    QVERIFY(m_repository->save(makeAccount(
        QUuid::createUuid(), QStringLiteral("现金"))));
    QVERIFY(m_repository->save(makeAccount(
        QUuid::createUuid(), QStringLiteral("银行卡"))));

    const auto accounts = m_repository->findAll();

    QVERIFY2(m_repository->lastError().isEmpty(),
             qPrintable(m_repository->lastError()));
    QCOMPARE(accounts.size(), std::size_t{2});
}

void SqliteAccountRepositoryTest::worksWithCreateAccountUseCase()
{
    CreateAccount createAccount(*m_repository);

    const auto result = createAccount.execute(CreateAccountRequest{
        .name = QStringLiteral("支付宝"),
        .openingBalance = Money::fromCents(8800),
    });

    QVERIFY(std::holds_alternative<Account>(result));
    const auto &created = std::get<Account>(result);

    const auto stored = m_repository->findById(created.id());
    QVERIFY2(stored.has_value(), qPrintable(m_repository->lastError()));
    QCOMPARE(stored->name(), QStringLiteral("支付宝"));
    QCOMPARE(stored->openingBalance().cents(), qint64{8800});
}

QTEST_GUILESS_MAIN(SqliteAccountRepositoryTest)

#include "SqliteAccountRepositoryTest.moc"
```

### 为什么 cleanup 顺序不能交换

必须先销毁 Repository，再销毁 `SqliteDatabase`：

```text
SqliteAccountRepository 释放 QSqlDatabase 句柄
  -> SqliteDatabase 关闭并 removeDatabase
  -> QTemporaryDir 删除数据库文件
```

如果数据库先析构，而 Repository 仍持有同一连接，Qt 可能输出：

```text
connection is still in use
```

## 12. 注册新的集成测试目标

在 `tests/integration/accounting/CMakeLists.txt` 原有目标之后加入：

```cmake
add_executable(StuKitSqliteAccountRepositoryTest
    SqliteAccountRepositoryTest.cpp
)

target_link_libraries(StuKitSqliteAccountRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingApplication
        StuKitAccountingInfrastructure
)

add_test(
    NAME accounting.sqlite_account_repository
    COMMAND StuKitSqliteAccountRepositoryTest
)
```

测试目标同时链接 Application 和 Infrastructure，是为了验证：

```text
CreateAccount -> IAccountRepository -> SqliteAccountRepository -> SQLite
```

产品层之间仍然没有反向依赖；只有位于最外层的集成测试负责组装它们。

## 13. 构建和运行测试

在 Qt Creator 中：

1. 选择 `Desktop Qt 6.10.1 MSVC2022 64bit` Kit。
2. 执行“构建 > 运行 CMake”。
3. 执行“构建 > 重新构建项目”。
4. 打开“测试结果”。
5. 运行全部测试，不要只运行新增测试。

预期新增测试名：

```text
accounting.sqlite_account_repository
```

最终应有 8 组测试：

```text
accounting.money
accounting.transaction
accounting.account
accounting.category
accounting.account_repository
accounting.create_account
accounting.sqlite_database
accounting.sqlite_account_repository
```

## 14. 人工理解检查

完成代码后，请确认自己可以回答：

1. 为什么 `SqliteAccountRepository` 放在 Infrastructure？
2. 为什么 `IAccountRepository` 不能包含 `QSqlDatabase`？
3. 为什么 SQL 需要 `bindValue()`，不能拼接账户名称？
4. 为什么从数据库读取后还要调用 `Account::create()`？
5. 为什么 `CreateAccount` 不需要知道它使用的是内存还是 SQLite？
6. 为什么测试清理时 Repository 必须先于数据库销毁？

## 15. 常见错误

### `QSqlQuery` does not name a type

确认 `SqliteAccountRepository.h` 在 namespace 之前有：

```cpp
class QSqlQuery;
```

也可以直接包含 `<QSqlQuery>`，但前置声明可以减少头文件依赖。

### `std::variant` 或 `std::holds_alternative` 未定义

使用它们的文件必须直接包含：

```cpp
#include <variant>
```

不要依赖其他 Qt 或标准库头文件间接包含它。

### `no such table: accounts`

创建 Repository 前必须成功执行：

```cpp
database.open(path);
database.migrate();
```

Repository 不会自行迁移数据库。

### `Parameter count mismatch` 或绑定值无效

检查 SQL 中的参数名和 `bindValue()` 名称是否完全一致，包括开头的冒号。

### `connection is still in use`

确认 Repository 在 `SqliteDatabase` 之前析构。测试的 `cleanup()` 顺序不能颠倒。

### 新测试没有出现在 Qt Creator 中

确认：

- 测试 `.cpp` 已加入新的 `add_executable()`。
- 已调用 `add_test()`。
- 已重新运行 CMake。
- 文件没有被错误加入 `qt_add_qml_module()`。

### 测试运行后没有终端输出

数据库测试必须使用 `QTEST_GUILESS_MAIN`，并确保 Qt 的 `bin` 目录在测试运行环境
的 `PATH` 中。Qt Creator 使用正确 Kit 时会自动设置运行环境。

## 16. 本课验收清单

- [x] `CreateAccount.h` 直接包含 `<variant>`。
- [x] `SqliteAccountRepository` 实现 `IAccountRepository`。
- [x] `save()` 使用参数绑定，没有拼接用户数据。
- [x] 相同 ID 再次保存会更新，而不是新增第二行。
- [x] UUID、名称、期初余额和归档状态可以完整往返数据库。
- [x] 缺失 ID 返回 `std::nullopt`，且不记录数据库错误。
- [x] `findAll()` 使用明确的 SQL 排序。
- [x] 从数据库恢复账户时仍调用 `Account::create()`。
- [x] `CreateAccount` 可以通过 SQLite Repository 保存账户。
- [x] 集成测试正确管理 Repository 和数据库析构顺序。
- [x] 8 组测试全部通过。
- [x] StuKit 应用仍能正常构建和运行。
- [x] `git diff --check` 没有输出。
- [x] Git 状态中没有数据库文件和 build 产物。

## 17. 完成后反馈

先不要提交代码。完成后把下面的信息发给我：

```text
CMake 配置：成功 / 失败
StuKitAccountingInfrastructure：成功 / 失败
SqliteAccountRepositoryTest：成功 / 失败
8 组测试结果：
StuKit 应用：正常 / 异常
运行警告：
git diff --stat 输出：
```

我会检查 Repository 实现、SQL、对象生命周期和测试结果，通过后再帮你整理格式、
更新文档并创建 Git 提交。
