# 第七课：Account 账户实体

## 本课目标

本课实现记账模块的 `Account` 实体，用它表示现金、银行卡等资金账户。

完成后，你应该理解：

- 如何为领域对象定义清晰且可测试的输入规则。
- 为什么界面校验之外仍然需要领域校验。
- 为什么期初余额允许为负数。
- 为什么已经被账单引用的账户应该归档，而不是直接删除。
- 如何用测试辅助函数减少重复代码。

本课不计算实时余额，不连接 SQLite，也不把 Account 暴露给 QML。

## 1. Account 负责什么

第一版 Account 只保存四项数据：

```text
id              账户的稳定 ID
name            展示名称
openingBalance  期初余额
archived        是否已经归档
```

实时余额不直接保存在 Account 中，将来由“期初余额 + 收入 - 支出”计算得到。
这样可以避免账户余额和账单流水互相矛盾。

账户名称采用以下规则：

- 去除首尾空白。
- 处理后不能为空。
- 最长 40 个字符。

期初余额可以是正数、零或负数。信用账户或欠款账户可能从负余额开始，因此不能
像 Transaction 金额一样要求它必须大于零。

## 2. 为什么使用归档而不是删除

一笔 Transaction 会保存 `accountId`。如果直接删除已经被账单引用的账户，历史
账单就无法再显示账户信息。

本项目后续采用归档策略：

- 活跃账户可以用于新账单。
- 归档账户不再出现在新账单的默认选择列表中。
- 历史账单仍然能够找到归档账户。

本课只保存归档状态，修改和筛选归档账户会在用例层实现。

## 3. 本课需要修改的文件

新增：

```text
src/features/accounting/domain/Account.h
src/features/accounting/domain/Account.cpp
tests/unit/accounting/AccountTest.cpp
```

修改：

```text
src/features/accounting/domain/CMakeLists.txt
tests/unit/accounting/CMakeLists.txt
```

不要把这些 C++ 文件添加到根目录的 `STUKIT_QML_FILES`。它们属于领域库和测试
目标，不属于 QML 模块。

## 4. 创建 Account.h

创建 `src/features/accounting/domain/Account.h`：

```cpp
#pragma once

#include "Money.h"

#include <QString>
#include <QUuid>

#include <optional>

namespace stukit::accounting {

struct AccountData
{
    QUuid id;
    QString name;
    Money openingBalance{Money::fromCents(0)};
    bool archived{false};
};

class Account final
{
public:
    static constexpr qsizetype MaximumNameLength = 40;

    static std::optional<Account> create(AccountData data);

    [[nodiscard]] const QUuid &id() const noexcept;
    [[nodiscard]] const QString &name() const noexcept;
    [[nodiscard]] Money openingBalance() const noexcept;
    [[nodiscard]] bool isArchived() const noexcept;

private:
    explicit Account(AccountData data);

    AccountData m_data;
};

} // namespace stukit::accounting
```

这里继续使用工厂函数和私有构造函数。调用者不能绕过 `create` 直接产生一个名称
为空的 Account。

`MaximumNameLength` 放在 Account 上，让测试、用例和未来的输入提示都能使用同一
条限制，避免在不同位置重复写数字 40。

## 5. 创建 Account.cpp

创建 `src/features/accounting/domain/Account.cpp`：

```cpp
#include "Account.h"

#include <utility>

namespace stukit::accounting {

std::optional<Account> Account::create(AccountData data)
{
    data.name = data.name.trimmed();

    if (data.id.isNull()
        || data.name.isEmpty()
        || data.name.size() > MaximumNameLength) {
        return std::nullopt;
    }

    return Account(std::move(data));
}

Account::Account(AccountData data)
    : m_data(std::move(data))
{
}

const QUuid &Account::id() const noexcept
{
    return m_data.id;
}

const QString &Account::name() const noexcept
{
    return m_data.name;
}

Money Account::openingBalance() const noexcept
{
    return m_data.openingBalance;
}

bool Account::isArchived() const noexcept
{
    return m_data.archived;
}

} // namespace stukit::accounting
```

先标准化名称，再检查长度和空值。这样 `"   "` 会在 `trimmed()` 后变成空字符串，
不能创建账户。

## 6. 将 Account 加入领域库

在 `src/features/accounting/domain/CMakeLists.txt` 的
`StuKitAccountingDomain` 源文件列表中加入：

```cmake
    Account.cpp
    Account.h
```

只修改这个领域库的源文件列表，不要新建第二个领域库。

完成后重新运行一次 CMake。如果 Qt Creator 自动修改了根 `CMakeLists.txt`，检查
并删除被错误加入 `STUKIT_QML_FILES` 的 C++ 文件。

## 7. 创建 AccountTest.cpp

创建 `tests/unit/accounting/AccountTest.cpp`：

```cpp
#include "Account.h"

#include <QTest>

using namespace stukit::accounting;

namespace {

AccountData validAccountData()
{
    return AccountData{
        .id = QUuid::createUuid(),
        .name = QStringLiteral("现金"),
        .openingBalance = Money::fromCents(10000),
        .archived = false,
    };
}

} // namespace

class AccountTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsValidAccount();
    void rejectsNullId();
    void rejectsInvalidName_data();
    void rejectsInvalidName();
    void allowsNegativeOpeningBalance();
    void preservesArchivedState();
};

void AccountTest::createsValidAccount()
{
    auto data = validAccountData();
    data.name = QStringLiteral("  现金  ");

    const auto account = Account::create(data);

    QVERIFY(account.has_value());
    QCOMPARE(account->id(), data.id);
    QCOMPARE(account->name(), QStringLiteral("现金"));
    QVERIFY(account->openingBalance() == Money::fromCents(10000));
    QVERIFY(!account->isArchived());
}

void AccountTest::rejectsNullId()
{
    auto data = validAccountData();
    data.id = QUuid{};

    QVERIFY(!Account::create(data).has_value());
}

void AccountTest::rejectsInvalidName_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("empty") << QString{};
    QTest::newRow("only whitespace") << QStringLiteral("   ");
    QTest::newRow("too long")
        << QString(Account::MaximumNameLength + 1, QLatin1Char('a'));
}

void AccountTest::rejectsInvalidName()
{
    QFETCH(QString, name);

    auto data = validAccountData();
    data.name = name;

    QVERIFY(!Account::create(data).has_value());
}

void AccountTest::allowsNegativeOpeningBalance()
{
    auto data = validAccountData();
    data.openingBalance = Money::fromCents(-5000);

    const auto account = Account::create(data);

    QVERIFY(account.has_value());
    QVERIFY(account->openingBalance() == Money::fromCents(-5000));
}

void AccountTest::preservesArchivedState()
{
    auto data = validAccountData();
    data.archived = true;

    const auto account = Account::create(data);

    QVERIFY(account.has_value());
    QVERIFY(account->isArchived());
}

QTEST_APPLESS_MAIN(AccountTest)

#include "AccountTest.moc"
```

`validAccountData()` 是测试辅助函数。每个测试先获得一份有效数据，再只改变自己
需要验证的字段，这样失败时更容易判断是哪条规则出了问题。

## 8. 创建独立测试目标

在 `tests/unit/accounting/CMakeLists.txt` 末尾加入：

```cmake
add_executable(StuKitAccountTest
    AccountTest.cpp
)

target_link_libraries(StuKitAccountTest
    PRIVATE
        Qt6::Test
        StuKitAccountingDomain
)

add_test(
    NAME accounting.account
    COMMAND StuKitAccountTest
)
```

不要把 `AccountTest.cpp` 放入 Money 或 Transaction 的测试目标。每个包含
`QTEST_APPLESS_MAIN` 的文件都需要独立的可执行程序。

## 9. 构建与运行测试

在 Qt Creator 中：

1. 运行 CMake。
2. 构建整个项目。
3. 打开 Tests 面板。
4. 运行所有测试。

预期至少看到：

```text
accounting.money          Passed
accounting.transaction    Passed
accounting.account        Passed
```

也可以从配置好 Qt Kit 环境的终端执行：

```powershell
ctest --test-dir <你的构建目录> --output-on-failure
```

## 10. 人工检查

本课没有修改界面，但仍要运行一次 StuKit，确认：

- 应用正常启动。
- 首页和两个功能页仍能切换。
- 主题切换仍然正常。
- 应用输出中没有新增 QML 错误。

这是在检查新增领域代码没有意外破坏现有应用目标。

## 11. 常见错误

### qsizetype 未定义

确认 `Account.h` 已经包含 `Money.h`。Money 使用 Qt 基础类型，因此当前目标已经
链接 `Qt6::Core`。如果编译器仍然无法识别，也可以显式加入 `#include <QtTypes>`。

### designated initializer 的顺序错误

C++20 的指定初始化器必须按照 `AccountData` 中字段的声明顺序书写：`id`、
`name`、`openingBalance`、`archived`。

### AccountTest.moc 找不到

确认测试类中存在 `Q_OBJECT`，文件末尾是：

```cpp
#include "AccountTest.moc"
```

然后重新运行 CMake，而不只是重复点击构建。

### 只有原来的两组测试

确认 `tests/unit/accounting/CMakeLists.txt` 同时创建了 `StuKitAccountTest`，并通过
`add_test(NAME accounting.account ...)` 注册。修改 CMake 后需要重新运行 CMake。

### 链接器报告 main 或测试元对象重复定义

检查 `AccountTest.cpp` 是否同时出现在多个 `add_executable` 中。它包含
`QTEST_APPLESS_MAIN(AccountTest)`，必须只属于 `StuKitAccountTest`，不能再加入
`StuKitMoneyTest` 或 `StuKitTransactionTest`。

## 12. 本课验收清单

- [ ] Account 拥有稳定且非空的 UUID。
- [ ] 名称会去除首尾空白。
- [ ] 空名称、纯空白名称和超长名称会被拒绝。
- [ ] 期初余额允许为正数、零或负数。
- [ ] Account 能保存归档状态。
- [ ] Account 属于 StuKitAccountingDomain。
- [ ] 三组记账领域测试全部通过。
- [ ] StuKit 应用仍能正常运行。
- [ ] 根 QML 模块没有混入 Account 或测试源码。
- [ ] Git 状态中没有构建产物。

## 13. 完成后反馈

完成代码后，请把以下信息发给我：

```text
CMake 配置：成功 / 失败
StuKitAccountTest：成功 / 失败
三组测试结果：
StuKit 应用：正常 / 异常
应用或测试警告：
git diff --stat 输出：
```

先不要提交代码。我会检查实现、解释问题，并在验收通过后帮助你整理本课提交。
