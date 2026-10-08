# 第九课：Account Repository 接口与测试替身

## 本课目标

本课为 Account 定义第一个 Repository 接口，并在测试代码中实现一个只保存在
内存中的测试替身。

完成后，你应该理解：

- Repository 解决什么问题。
- 接口与具体实现的区别。
- 什么是纯虚函数和抽象类。
- 为什么 Repository 接口属于 Domain，而 SQLite 实现属于 Infrastructure。
- 什么是依赖倒置。
- 为什么应用用例测试不应该依赖真实数据库。

本课不连接 SQLite，也不修改 QML 页面。

## 1. Repository 是什么

领域代码需要表达“保存账户”“按 ID 查找账户”和“列出账户”，但它不应该知道
数据最终保存到 SQLite、内存还是其他设备。

我们先定义一个接口：

```text
IAccountRepository
  save(account)
  findById(id)
  findAll()
```

调用关系为：

```text
Application 用例 -> IAccountRepository <- SQLite 实现
                                   ^
                                   |
                              测试内存实现
```

箭头两边的实现都依赖同一个抽象接口。Application 不创建 SQLite 对象，因此以后
迁移 Android 时，业务用例不需要跟随存储细节一起重写。

## 2. 为什么先只做 Account Repository

Account、Category 和 Transaction 最终都会有 Repository，但第一次学习接口时，
同时编写三套几乎相同的代码只会增加出错位置。

本课先完成 Account 的完整最小路径。后续实现 Category 和 Transaction Repository
时会复用相同原则，并根据查询需求增加不同方法。

## 3. 接口约定

本课的接口约定如下：

- `save`：相同 ID 已存在时替换，否则新增；成功返回 `true`。
- `findById`：找到时返回 Account，未找到时返回 `std::nullopt`。
- `findAll`：返回全部账户，包括归档账户。
- 返回顺序暂时不作为接口保证。

目前使用 `bool` 和 `std::optional` 保持第一版接口容易理解。连接 SQLite 时需要区分
“没有找到”和“数据库失败”，届时会引入统一错误结果，而不是把错误字符串写入
Domain。

Repository 不负责判断 Account 是否有效。它只接收已经通过 `Account::create`
创建成功的对象。

## 4. 本课文件变化

新增：

```text
src/features/accounting/domain/IAccountRepository.h
tests/unit/accounting/support/InMemoryAccountRepository.h
tests/unit/accounting/AccountRepositoryTest.cpp
```

修改：

```text
src/features/accounting/domain/CMakeLists.txt
tests/unit/accounting/CMakeLists.txt
```

`InMemoryAccountRepository` 只服务自动测试，所以必须放在 `tests/`，不能放进产品的
Domain 或 Infrastructure 目录。

## 5. 创建 IAccountRepository.h

创建 `src/features/accounting/domain/IAccountRepository.h`：

```cpp
#pragma once

#include "Account.h"

#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::accounting {

class IAccountRepository
{
public:
    virtual ~IAccountRepository() = default;

    [[nodiscard]] virtual bool save(const Account &account) = 0;
    [[nodiscard]] virtual std::optional<Account> findById(const QUuid &id) const = 0;
    [[nodiscard]] virtual std::vector<Account> findAll() const = 0;
};

} // namespace stukit::accounting
```

重点：

- 类名开头的 `I` 表示它是接口，这是本项目采用的命名约定。
- `= 0` 表示纯虚函数，IAccountRepository 不能被直接实例化。
- 虚析构函数确保通过接口指针销毁实现类时行为正确。
- `const` 表示查询方法不应修改 Repository 的可观察状态。
- `[[nodiscard]]` 提醒调用者不要忽略保存或查询结果。

不要在这个头文件中包含 `QSqlDatabase`、`QSqlQuery` 或 SQLite 头文件。Domain 只描述
需要的能力，不描述实现方式。

## 6. 将接口加入 Domain 目标

在 `src/features/accounting/domain/CMakeLists.txt` 的源文件列表中加入：

```cmake
    IAccountRepository.h
```

这是只有头文件的抽象接口，不需要创建 `IAccountRepository.cpp`。

## 7. 创建测试支持目录

在 `tests/unit/accounting` 下创建：

```text
support/
```

然后创建 `support/InMemoryAccountRepository.h`：

```cpp
#pragma once

#include "IAccountRepository.h"

#include <algorithm>
#include <vector>

namespace stukit::accounting::testing {

class InMemoryAccountRepository final : public IAccountRepository
{
public:
    bool save(const Account &account) override
    {
        const auto existing = std::find_if(
            m_accounts.begin(),
            m_accounts.end(),
            [&account](const Account &candidate) {
                return candidate.id() == account.id();
            });

        if (existing == m_accounts.end()) {
            m_accounts.push_back(account);
        } else {
            *existing = account;
        }

        return true;
    }

    std::optional<Account> findById(const QUuid &id) const override
    {
        const auto account = std::find_if(
            m_accounts.cbegin(),
            m_accounts.cend(),
            [&id](const Account &candidate) {
                return candidate.id() == id;
            });

        if (account == m_accounts.cend()) {
            return std::nullopt;
        }

        return *account;
    }

    std::vector<Account> findAll() const override
    {
        return m_accounts;
    }

private:
    std::vector<Account> m_accounts;
};

} // namespace stukit::accounting::testing
```

这个类没有 SQL、文件或网络。数据只存在 `m_accounts` 中，测试进程结束后自动
消失，因此测试运行快、互不污染。

`override` 会让编译器检查函数签名是否真的覆盖接口。如果参数、返回值或 `const`
写错，编译器会直接指出问题。

## 8. 创建 AccountRepositoryTest.cpp

创建 `tests/unit/accounting/AccountRepositoryTest.cpp`：

```cpp
#include "support/InMemoryAccountRepository.h"

#include <QTest>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

namespace {

Account createAccount(const QUuid &id, const QString &name)
{
    const auto account = Account::create(AccountData{
        .id = id,
        .name = name,
        .openingBalance = Money::fromCents(0),
        .archived = false,
    });

    Q_ASSERT(account.has_value());
    return *account;
}

} // namespace

class AccountRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void savesAndFindsAccount();
    void returnsNulloptForMissingAccount();
    void updatesAccountWithSameId();
    void returnsAllAccounts();
};

void AccountRepositoryTest::savesAndFindsAccount()
{
    InMemoryAccountRepository repository;
    const auto id = QUuid::createUuid();
    const auto account = createAccount(id, QStringLiteral("现金"));

    QVERIFY(repository.save(account));

    const auto stored = repository.findById(id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->id(), id);
    QCOMPARE(stored->name(), QStringLiteral("现金"));
}

void AccountRepositoryTest::returnsNulloptForMissingAccount()
{
    const InMemoryAccountRepository repository;

    const auto stored = repository.findById(QUuid::createUuid());

    QVERIFY(!stored.has_value());
}

void AccountRepositoryTest::updatesAccountWithSameId()
{
    InMemoryAccountRepository repository;
    const auto id = QUuid::createUuid();

    QVERIFY(repository.save(createAccount(id, QStringLiteral("现金"))));
    QVERIFY(repository.save(createAccount(id, QStringLiteral("日常账户"))));

    const auto accounts = repository.findAll();
    QCOMPARE(accounts.size(), std::size_t{1});
    QCOMPARE(accounts.front().name(), QStringLiteral("日常账户"));
}

void AccountRepositoryTest::returnsAllAccounts()
{
    InMemoryAccountRepository repository;

    QVERIFY(repository.save(
        createAccount(QUuid::createUuid(), QStringLiteral("现金"))));
    QVERIFY(repository.save(
        createAccount(QUuid::createUuid(), QStringLiteral("银行卡"))));

    const auto accounts = repository.findAll();

    QCOMPARE(accounts.size(), std::size_t{2});
}

QTEST_APPLESS_MAIN(AccountRepositoryTest)

#include "AccountRepositoryTest.moc"
```

测试辅助函数 `createAccount` 中使用 `Q_ASSERT`，因为传入的是测试自己控制的有效
数据。如果这里失败，说明测试准备代码写错了，而不是被测试行为返回了正常失败。

## 9. 创建第五个测试目标

在 `tests/unit/accounting/CMakeLists.txt` 末尾加入：

```cmake
add_executable(StuKitAccountRepositoryTest
    AccountRepositoryTest.cpp
    support/InMemoryAccountRepository.h
)

target_link_libraries(StuKitAccountRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingDomain
)

add_test(
    NAME accounting.account_repository
    COMMAND StuKitAccountRepositoryTest
)
```

把测试支持头文件列进目标，可以让 Qt Creator 更容易展示它。它是头文件，不会
单独生成第二份实现。

`AccountRepositoryTest.cpp` 只能属于这个测试目标。

## 10. 这次依赖方向为什么正确

```text
StuKitAccountingDomain
  Account
  IAccountRepository

StuKitAccountRepositoryTest
  -> StuKitAccountingDomain
  -> InMemoryAccountRepository（仅测试）
```

Domain 不知道测试替身存在。未来的 SQLite 实现也会依赖
`StuKitAccountingDomain`，而不是让 Domain 依赖 Qt SQL。

这就是依赖倒置：高层业务规则依赖抽象，底层存储细节实现抽象。

## 11. 构建和测试

在 Qt Creator 中：

1. 运行 CMake。
2. 构建整个项目。
3. 在 Tests 面板运行全部测试。
4. 启动 StuKit，确认现有界面正常。

预期出现五组测试：

```text
accounting.money                 Passed
accounting.transaction           Passed
accounting.account               Passed
accounting.category              Passed
accounting.account_repository    Passed
```

## 12. 常见错误

### cannot instantiate abstract class

说明 InMemoryAccountRepository 没有正确覆盖全部纯虚函数。重点检查参数类型、返回
类型以及查询函数末尾的 `const` 是否与接口完全一致。

### function with override specifier did not override

同样表示实现函数签名与接口不一致。不要删除 `override` 来绕过错误，应对照
IAccountRepository 修正签名。

### InMemoryAccountRepository.h 找不到

在 AccountRepositoryTest.cpp 中使用：

```cpp
#include "support/InMemoryAccountRepository.h"
```

源文件位于 `tests/unit/accounting`，相对路径从该目录开始。

### std::find_if 未定义

确认测试替身头文件包含 `<algorithm>`。

### std::size_t 或 vector 未定义

确认包含 `<vector>`。`std::vector<Account>::size()` 返回无符号的 `size_type`，测试
中使用 `std::size_t{1}` 和 `std::size_t{2}` 避免有符号比较警告。

### 保存后出现两个相同 ID 的账户

检查 `save` 是否先使用 `find_if` 查找 ID。找到时应执行 `*existing = account`，只有
未找到时才 `push_back`。

## 13. 本课验收清单

- [ ] IAccountRepository 是只有纯虚函数的抽象接口。
- [ ] 接口没有包含 Qt SQL 或 SQLite 类型。
- [ ] 接口拥有虚析构函数。
- [ ] save、findById 和 findAll 的契约清楚。
- [ ] InMemoryAccountRepository 只位于 tests 目录。
- [ ] 保存新 ID 会新增账户。
- [ ] 保存相同 ID 会替换账户，而不是重复新增。
- [ ] 查找未知 ID 返回 std::nullopt。
- [ ] findAll 能返回全部账户。
- [ ] 五组测试全部通过。
- [ ] StuKit 应用仍能正常运行。
- [ ] 根 QML 模块没有混入 Repository 或测试文件。
- [ ] Git 状态中没有构建产物。

## 14. 完成后反馈

完成后先不要提交代码，请把以下信息发给我：

```text
CMake 配置：成功 / 失败
StuKitAccountRepositoryTest：成功 / 失败
五组测试结果：
StuKit 应用：正常 / 异常
应用或测试警告：
git diff --stat 输出：
```

下一课会基于这个接口建立独立的 Application 目标，并实现第一个账户用例。
