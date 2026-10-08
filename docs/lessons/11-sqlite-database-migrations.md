# 第十一课：SQLite 连接与数据库迁移

## 本课目标

本课开始建立 Infrastructure 层，使用 Qt SQL 打开 SQLite 数据库，并实现第一版
数据库迁移机制。

完成后，你应该理解：

- SQLite、Qt SQL 和 Repository 各自负责什么。
- 为什么数据库连接属于 Infrastructure。
- 为什么表结构必须通过有版本号的迁移修改。
- 如何让迁移可以安全地重复执行。
- 如何使用命名连接隔离测试。
- 如何用临时目录完成数据库集成测试。

本课只建立数据库和 `accounts` 表，不实现 `SqliteAccountRepository`，也不会读写
正式用户数据。

## 1. 为什么不能直接在 Repository 中临时建表

如果每个 Repository 在查询前随手执行 `CREATE TABLE`，数据库结构会分散在多个
类里，很难知道一个已有数据库应该升级到什么状态。

本项目使用顺序迁移：

```text
空数据库
  -> migration 1：创建 accounts 表
  -> migration 2：以后创建 categories 表
  -> migration 3：以后创建 transactions 表
```

数据库用 `schema_migrations` 表记录已经执行的版本。应用启动时只执行尚未应用的
迁移。

已发布的迁移不能修改。表结构变化时必须新增更高版本，否则旧用户和新用户会
得到不同的数据库结构。

## 2. 本课的层次关系

```text
StuKitAccountingInfrastructure
  -> Qt6::Sql
  -> StuKitAccountingDomain

SqliteDatabaseTest
  -> StuKitAccountingInfrastructure
  -> 临时 SQLite 文件
```

Domain 和 Application 不包含 `QSqlDatabase` 或 `QSqlQuery`。将来 Android 版本仍然
可以复用这两个上层目标，平台路径差异集中在应用组装和 Infrastructure。

## 3. 本课文件变化

新增：

```text
src/features/accounting/infrastructure/CMakeLists.txt
src/features/accounting/infrastructure/database/SqliteDatabase.h
src/features/accounting/infrastructure/database/SqliteDatabase.cpp
tests/integration/accounting/CMakeLists.txt
tests/integration/accounting/SqliteDatabaseTest.cpp
```

修改：

```text
CMakeLists.txt
tests/CMakeLists.txt
```

## 4. 在根工程启用 Qt SQL

将根 `CMakeLists.txt` 中：

```cmake
find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)
```

改为：

```cmake
find_package(Qt6 6.8 REQUIRED COMPONENTS Quick Sql)
```

然后在 Application 子目录之后加入：

```cmake
add_subdirectory(src/features/accounting/infrastructure)
```

顺序应为：

```cmake
add_subdirectory(src/features/accounting/domain)
add_subdirectory(src/features/accounting/application)
add_subdirectory(src/features/accounting/infrastructure)
```

不要把 Infrastructure 文件加入 `STUKIT_QML_FILES`。

## 5. 创建 Infrastructure 目标

创建目录：

```text
src/features/accounting/infrastructure/database/
```

在 `src/features/accounting/infrastructure/CMakeLists.txt` 中写入：

```cmake
add_library(StuKitAccountingInfrastructure STATIC
    database/SqliteDatabase.cpp
    database/SqliteDatabase.h
)

target_include_directories(StuKitAccountingInfrastructure
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitAccountingInfrastructure
    PUBLIC
        Qt6::Sql
        StuKitAccountingDomain
)

target_compile_features(StuKitAccountingInfrastructure
    PUBLIC
        cxx_std_20
)
```

Infrastructure 可以依赖 Domain，因为下一课的 SQLite Repository 要实现 Domain
接口；Domain 不能反向链接 Infrastructure。

## 6. 创建 SqliteDatabase.h

创建 `src/features/accounting/infrastructure/database/SqliteDatabase.h`：

```cpp
#pragma once

#include <QSqlDatabase>
#include <QString>

namespace stukit::accounting {

class SqliteDatabase final
{
public:
    explicit SqliteDatabase(QString connectionName);
    ~SqliteDatabase();

    SqliteDatabase(const SqliteDatabase &) = delete;
    SqliteDatabase &operator=(const SqliteDatabase &) = delete;
    SqliteDatabase(SqliteDatabase &&) = delete;
    SqliteDatabase &operator=(SqliteDatabase &&) = delete;

    [[nodiscard]] bool open(const QString &filePath);
    [[nodiscard]] bool migrate();

    [[nodiscard]] QSqlDatabase connection() const;
    [[nodiscard]] const QString &lastError() const noexcept;

private:
    [[nodiscard]] bool ensureMigrationTable();
    [[nodiscard]] bool readCurrentVersion(int &version);
    [[nodiscard]] bool applyMigration1();

    QString m_connectionName;
    QSqlDatabase m_database;
    QString m_lastError;
};

} // namespace stukit::accounting
```

禁止复制和移动这个对象，是为了避免多个对象误以为自己拥有同一个命名连接。
`QSqlDatabase` 本身是共享句柄，但连接的创建和移除必须有清晰的唯一所有者。

`connection()` 返回一个轻量句柄，下一课的 Repository 会用它创建 `QSqlQuery`。

## 7. 创建 SqliteDatabase.cpp

创建 `src/features/accounting/infrastructure/database/SqliteDatabase.cpp`：

```cpp
#include "database/SqliteDatabase.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>

#include <utility>

namespace stukit::accounting {

SqliteDatabase::SqliteDatabase(QString connectionName)
    : m_connectionName(std::move(connectionName))
    , m_database(QSqlDatabase::addDatabase(
          QStringLiteral("QSQLITE"), m_connectionName))
{
}

SqliteDatabase::~SqliteDatabase()
{
    if (m_database.isValid()) {
        m_database.close();
    }

    m_database = QSqlDatabase{};
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool SqliteDatabase::open(const QString &filePath)
{
    m_lastError.clear();
    m_database.setDatabaseName(filePath);

    if (!m_database.open()) {
        m_lastError = m_database.lastError().text();
        return false;
    }

    QSqlQuery foreignKeys(m_database);
    if (!foreignKeys.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
        m_lastError = foreignKeys.lastError().text();
        return false;
    }

    return true;
}

bool SqliteDatabase::migrate()
{
    m_lastError.clear();

    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }

    if (!ensureMigrationTable()) {
        return false;
    }

    int version = 0;
    if (!readCurrentVersion(version)) {
        return false;
    }

    if (version < 1 && !applyMigration1()) {
        return false;
    }

    return true;
}

QSqlDatabase SqliteDatabase::connection() const
{
    return m_database;
}

const QString &SqliteDatabase::lastError() const noexcept
{
    return m_lastError;
}

bool SqliteDatabase::ensureMigrationTable()
{
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS schema_migrations ("
            "version INTEGER PRIMARY KEY NOT NULL, "
            "applied_at TEXT NOT NULL)"))) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool SqliteDatabase::readCurrentVersion(int &version)
{
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT COALESCE(MAX(version), 0) FROM schema_migrations"))) {
        m_lastError = query.lastError().text();
        return false;
    }

    if (!query.next()) {
        m_lastError = QStringLiteral("Could not read schema version");
        return false;
    }

    version = query.value(0).toInt();
    return true;
}

bool SqliteDatabase::applyMigration1()
{
    if (!m_database.transaction()) {
        m_lastError = m_database.lastError().text();
        return false;
    }

    QSqlQuery createAccounts(m_database);
    if (!createAccounts.exec(QStringLiteral(
            "CREATE TABLE accounts ("
            "id TEXT PRIMARY KEY NOT NULL, "
            "name TEXT NOT NULL, "
            "opening_balance_cents INTEGER NOT NULL, "
            "archived INTEGER NOT NULL DEFAULT 0 "
            "CHECK (archived IN (0, 1)))"))) {
        m_lastError = createAccounts.lastError().text();
        m_database.rollback();
        return false;
    }

    QSqlQuery recordMigration(m_database);
    recordMigration.prepare(QStringLiteral(
        "INSERT INTO schema_migrations(version, applied_at) "
        "VALUES(:version, :appliedAt)"));
    recordMigration.bindValue(QStringLiteral(":version"), 1);
    recordMigration.bindValue(
        QStringLiteral(":appliedAt"),
        QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));

    if (!recordMigration.exec()) {
        m_lastError = recordMigration.lastError().text();
        m_database.rollback();
        return false;
    }

    if (!m_database.commit()) {
        m_lastError = m_database.lastError().text();
        m_database.rollback();
        return false;
    }

    return true;
}

} // namespace stukit::accounting
```

迁移 1 在事务中同时创建表并记录版本。任何一步失败都会回滚，避免出现“表已经
创建，但版本没有记录”的半完成状态。

## 8. 理解 accounts 表

第一版表结构：

```text
id                     TEXT     UUID 字符串
name                   TEXT     账户名称
opening_balance_cents  INTEGER  期初余额，单位为分
archived               INTEGER  0 或 1
```

SQLite 没有专门的布尔存储类，本项目使用整数 0 和 1，并通过 `CHECK` 约束阻止
其他值。

金额继续以整数分保存，不使用 `REAL`。

## 9. 建立集成测试目录

当前 Domain 和 Application 测试属于单元测试。SQLite 测试会加载真实驱动并创建
数据库文件，因此放入新的集成测试目录：

```text
tests/integration/accounting/
```

在 `tests/CMakeLists.txt` 中加入：

```cmake
add_subdirectory(integration/accounting)
```

最终内容为：

```cmake
add_subdirectory(unit/accounting)
add_subdirectory(integration/accounting)
```

## 10. 创建集成测试 CMakeLists.txt

创建 `tests/integration/accounting/CMakeLists.txt`：

```cmake
add_executable(StuKitSqliteDatabaseTest
    SqliteDatabaseTest.cpp
)

target_link_libraries(StuKitSqliteDatabaseTest
    PRIVATE
        Qt6::Test
        StuKitAccountingInfrastructure
)

add_test(
    NAME accounting.sqlite_database
    COMMAND StuKitSqliteDatabaseTest
)
```

## 11. 创建 SqliteDatabaseTest.cpp

创建 `tests/integration/accounting/SqliteDatabaseTest.cpp`：

```cpp
#include "database/SqliteDatabase.h"

#include <QDir>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

using namespace stukit::accounting;

namespace {

QString uniqueConnectionName()
{
    return QStringLiteral("stukit-test-%1").arg(
        QUuid::createUuid().toString(QUuid::WithoutBraces));
}

QTemporaryDir createTemporaryDatabaseDirectory()
{
    return QTemporaryDir(
        QDir::current().filePath(QStringLiteral("stukit-sqlite-test-XXXXXX")));
}

} // namespace

class SqliteDatabaseTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void createsInitialSchema();
    void migrationIsIdempotent();
    void enablesForeignKeys();
};

void SqliteDatabaseTest::initTestCase()
{
    QVERIFY2(
        QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")),
        "QSQLITE driver is not available");
}

void SqliteDatabaseTest::createsInitialSchema()
{
    QTemporaryDir directory = createTemporaryDatabaseDirectory();
    QVERIFY(directory.isValid());

    SqliteDatabase database(uniqueConnectionName());
    QVERIFY2(
        database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
        qPrintable(database.lastError()));
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));

    QSet<QString> columns;
    {
        QSqlQuery query(database.connection());
        QVERIFY2(
            query.exec(QStringLiteral("PRAGMA table_info(accounts)")),
            qPrintable(query.lastError().text()));

        while (query.next()) {
            columns.insert(query.value(1).toString());
        }
    }

    QVERIFY(columns.contains(QStringLiteral("id")));
    QVERIFY(columns.contains(QStringLiteral("name")));
    QVERIFY(columns.contains(QStringLiteral("opening_balance_cents")));
    QVERIFY(columns.contains(QStringLiteral("archived")));
}

void SqliteDatabaseTest::migrationIsIdempotent()
{
    QTemporaryDir directory = createTemporaryDatabaseDirectory();
    QVERIFY(directory.isValid());

    SqliteDatabase database(uniqueConnectionName());
    QVERIFY2(
        database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
        qPrintable(database.lastError()));
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));

    {
        QSqlQuery query(database.connection());
        QVERIFY2(
            query.exec(QStringLiteral(
                "SELECT COUNT(*) FROM schema_migrations WHERE version = 1")),
            qPrintable(query.lastError().text()));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 1);
    }
}

void SqliteDatabaseTest::enablesForeignKeys()
{
    QTemporaryDir directory = createTemporaryDatabaseDirectory();
    QVERIFY(directory.isValid());

    SqliteDatabase database(uniqueConnectionName());
    QVERIFY2(
        database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
        qPrintable(database.lastError()));

    {
        QSqlQuery query(database.connection());
        QVERIFY2(
            query.exec(QStringLiteral("PRAGMA foreign_keys")),
            qPrintable(query.lastError().text()));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 1);
    }
}

QTEST_GUILESS_MAIN(SqliteDatabaseTest)

#include "SqliteDatabaseTest.moc"
```

每个测试使用不同的连接名和 `QTemporaryDir`。测试结束后临时数据库自动删除，不会
污染正式数据，也不会让不同测试互相依赖。

额外的大括号用于确保 `QSqlQuery` 在 SqliteDatabase 析构前销毁，避免 Qt 输出
“connection is still in use”警告。

## 12. 运行测试

在 Qt Creator 中：

1. 运行 CMake。
2. 确认 Qt SQL 和 `StuKitAccountingInfrastructure` 配置成功。
3. 构建整个项目。
4. 在 Tests 面板运行全部测试。
5. 启动 StuKit，确认现有界面仍然正常。

预期新增第七组测试：

```text
accounting.money                 Passed
accounting.transaction           Passed
accounting.account               Passed
accounting.category              Passed
accounting.account_repository    Passed
accounting.create_account        Passed
accounting.sqlite_database       Passed
```

## 13. 常见错误

### `integration/accounting` 中找不到 `CMakeLists.txt`

`tests/CMakeLists.txt` 使用的是：

```cmake
add_subdirectory(integration/accounting)
```

因此集成测试的构建文件必须放在
`tests/integration/accounting/CMakeLists.txt`，不能放在
`tests/integration/CMakeLists.txt`。

### SQLite 测试直接退出且没有测试日志

数据库测试需要使用 `QTEST_GUILESS_MAIN`。它会创建
`QCoreApplication`，让 Qt 能够通过插件系统加载 `QSQLITE` 驱动，同时不会
启动图形界面。`QTEST_APPLESS_MAIN` 不会创建应用程序对象，不适合这个集成测试。

### Could NOT find Qt6Sql

打开 Qt Maintenance Tool，确认 Qt 6.10.1 对应 MSVC 2022 64-bit Kit 安装了 Qt SQL
相关组件。不要混用 MinGW 的 Qt 库和 MSVC 编译器。

### QSQLITE driver not loaded

先检查应用或测试输出中的可用驱动。构建目录运行测试时需要能找到 Qt 的
`plugins/sqldrivers/qsqlite.dll`。从 Qt Creator Tests 面板运行通常会自动继承正确
环境。

### database connection is still in use

确保所有 `QSqlQuery` 和由 `connection()` 返回的临时数据库句柄，都在
SqliteDatabase 析构前离开作用域。测试中的额外大括号不能随意删除。

### 第二次 migrate 报 accounts already exists

检查 `readCurrentVersion` 是否正确读取 `MAX(version)`，以及迁移成功后是否向
`schema_migrations` 插入版本 1。迁移记录和建表必须在同一事务中提交。

### 测试数据库出现在仓库目录

测试必须使用 `QTemporaryDir::filePath`，不能把数据库文件名写死为仓库中的相对
路径。正式数据库位置以后通过 `QStandardPaths` 获取。

### Qt Creator 自动修改根 QML 模块

删除 `qt_add_qml_module` 中自动出现的 Infrastructure C++、CMake 或测试文件。
这些文件分别属于普通静态库和测试目标，不属于 QML 资源。

## 14. 本课验收清单

- [x] 根工程已经启用 Qt6::Sql。
- [x] 存在独立的 StuKitAccountingInfrastructure 静态库。
- [x] Domain 和 Application 没有依赖 Infrastructure。
- [x] 数据库使用唯一命名连接。
- [x] 数据库打开时启用 SQLite 外键。
- [x] schema_migrations 能记录迁移版本。
- [x] 迁移 1 在事务中创建 accounts 表。
- [x] accounts 金额字段使用 INTEGER。
- [x] archived 只允许 0 或 1。
- [x] migrate 可以重复执行且不会重复应用版本 1。
- [x] 集成测试只使用临时目录。
- [x] 七组测试全部通过。
- [x] StuKit 应用仍能正常运行。
- [x] Git 状态中没有数据库文件或构建产物。

## 15. 完成后反馈

完成后先不要提交代码，请把以下信息发给我：

```text
CMake 配置：成功 / 失败
QSQLITE 驱动：可用 / 不可用
StuKitAccountingInfrastructure：成功 / 失败
StuKitSqliteDatabaseTest：成功 / 失败
七组测试结果：
StuKit 应用：正常 / 异常
应用或测试警告：
git diff --stat 输出：
```

下一课会实现 `SqliteAccountRepository`，把当前的 IAccountRepository 接口真正连接
到本课创建的 accounts 表。
