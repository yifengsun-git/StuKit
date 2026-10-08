# 第十课：Application 层与 CreateAccount 用例

## 本课目标

本课建立记账模块的 Application 层，并实现第一个应用用例 `CreateAccount`。

完成后，你应该理解：

- Application 层与 Domain 层的职责区别。
- 什么是用例，以及为什么一个用例只完成一个用户目标。
- 如何通过构造函数注入 Repository 接口。
- 如何用 `std::variant` 表达成功或不同失败结果。
- 如何用内存 Repository 测试完整业务流程。
- 为什么 Application 只能依赖 Domain，不能依赖 SQLite 或 QML。

本课不连接数据库，也不制作账户管理页面。

## 1. 从实体规则进入业务流程

前面的 Account 负责保证单个账户有效，IAccountRepository 负责描述账户的保存和
查询能力。现在还缺少一个对象把这些步骤组织起来：

```text
接收创建账户请求
  -> 生成账户 UUID
  -> 调用 Account::create
  -> 调用 IAccountRepository::save
  -> 返回创建结果
```

这个流程就是 `CreateAccount` 用例。

用例不负责：

- 输入框和按钮。
- 将文本解析为金额。
- SQL 语句。
- 弹窗或错误提示文案。

这些职责分别属于 Presentation、Infrastructure 和 QML。

## 2. 本课的结果类型

创建账户有三种结果：

```text
Account                         创建并保存成功
CreateAccountError::InvalidInput   输入无法创建有效 Account
CreateAccountError::StorageFailure Repository 保存失败
```

使用：

```cpp
std::variant<Account, CreateAccountError>
```

`variant` 在任意时刻只保存其中一种类型，因此不会出现“既有成功账户又有失败
错误”这种互相矛盾的状态。

## 3. 本课文件变化

新增：

```text
src/features/accounting/application/CMakeLists.txt
src/features/accounting/application/CreateAccount.h
src/features/accounting/application/CreateAccount.cpp
tests/unit/accounting/CreateAccountTest.cpp
```

修改：

```text
CMakeLists.txt
tests/unit/accounting/support/InMemoryAccountRepository.h
tests/unit/accounting/CMakeLists.txt
```

## 4. 创建 Application 目录和目标

创建目录：

```text
src/features/accounting/application/
```

在其中创建 `CMakeLists.txt`：

```cmake
add_library(StuKitAccountingApplication STATIC
    CreateAccount.cpp
    CreateAccount.h
)

target_include_directories(StuKitAccountingApplication
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitAccountingApplication
    PUBLIC
        StuKitAccountingDomain
)

target_compile_features(StuKitAccountingApplication
    PUBLIC
        cxx_std_20
)
```

依赖方向只有：

```text
StuKitAccountingApplication -> StuKitAccountingDomain
```

Domain 的 CMakeLists.txt 不能链接 Application。

## 5. 将 Application 加入根工程

在根 `CMakeLists.txt` 中找到：

```cmake
add_subdirectory(src/features/accounting/domain)
```

紧接着加入：

```cmake
add_subdirectory(src/features/accounting/application)
```

它必须位于 `if(BUILD_TESTING)` 之前，因为测试目标稍后需要链接 Application。

不要把 Application 的 C++ 文件加入 `STUKIT_QML_FILES`。

## 6. 创建 CreateAccount.h

创建 `src/features/accounting/application/CreateAccount.h`：

```cpp
#pragma once

#include "IAccountRepository.h"
#include "Money.h"

#include <QString>

#include <variant>

namespace stukit::accounting {

struct CreateAccountRequest
{
    QString name;
    Money openingBalance{Money::fromCents(0)};
};

enum class CreateAccountError
{
    InvalidInput,
    StorageFailure
};

using CreateAccountResult = std::variant<Account, CreateAccountError>;

class CreateAccount final
{
public:
    explicit CreateAccount(IAccountRepository &repository);

    [[nodiscard]] CreateAccountResult execute(const CreateAccountRequest &request);

private:
    IAccountRepository &m_repository;
};

} // namespace stukit::accounting
```

构造函数接收引用，称为构造函数注入。CreateAccount 只知道
`IAccountRepository`，不知道运行时传入的是内存实现还是 SQLite 实现。

Repository 必须比 CreateAccount 活得更久，因为用例保存的是引用，不拥有
Repository。

## 7. 创建 CreateAccount.cpp

创建 `src/features/accounting/application/CreateAccount.cpp`：

```cpp
#include "CreateAccount.h"

#include <QUuid>

namespace stukit::accounting {

CreateAccount::CreateAccount(IAccountRepository &repository)
    : m_repository(repository)
{
}

CreateAccountResult CreateAccount::execute(const CreateAccountRequest &request)
{
    const auto account = Account::create(AccountData{
        .id = QUuid::createUuid(),
        .name = request.name,
        .openingBalance = request.openingBalance,
        .archived = false,
    });

    if (!account.has_value()) {
        return CreateAccountError::InvalidInput;
    }

    if (!m_repository.save(*account)) {
        return CreateAccountError::StorageFailure;
    }

    return *account;
}

} // namespace stukit::accounting
```

流程顺序很重要：只有 Account 创建成功后才调用 Repository。无效输入不应该触发
任何存储操作。

## 8. 让内存 Repository 模拟保存失败

为了测试 `StorageFailure`，修改
`tests/unit/accounting/support/InMemoryAccountRepository.h`。

在 `public:` 下、`save` 之前加入：

```cpp
    void setSaveShouldSucceed(bool shouldSucceed) noexcept
    {
        m_saveShouldSucceed = shouldSucceed;
    }
```

在 `save` 开头加入：

```cpp
        if (!m_saveShouldSucceed) {
            return false;
        }
```

在 `private:` 下加入：

```cpp
    bool m_saveShouldSucceed{true};
```

修改后的关键结构应为：

```cpp
class InMemoryAccountRepository final : public IAccountRepository
{
public:
    void setSaveShouldSucceed(bool shouldSucceed) noexcept
    {
        m_saveShouldSucceed = shouldSucceed;
    }

    bool save(const Account &account) override
    {
        if (!m_saveShouldSucceed) {
            return false;
        }

        // 原来的查找、插入或替换逻辑保持不变
    }

    // findById 和 findAll 保持不变

private:
    bool m_saveShouldSucceed{true};
    std::vector<Account> m_accounts;
};
```

默认值仍然为 `true`，所以第九课的 Repository 测试不需要修改，并能证明这次扩展
没有破坏原行为。

## 9. 创建 CreateAccountTest.cpp

创建 `tests/unit/accounting/CreateAccountTest.cpp`：

```cpp
#include "CreateAccount.h"
#include "support/InMemoryAccountRepository.h"

#include <QTest>

#include <variant>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

class CreateAccountTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsAndStoresValidAccount();
    void rejectsInvalidInputWithoutSaving();
    void reportsStorageFailure();
    void generatesDifferentIds();
};

void CreateAccountTest::createsAndStoresValidAccount()
{
    InMemoryAccountRepository repository;
    CreateAccount createAccount(repository);

    const auto result = createAccount.execute(CreateAccountRequest{
        .name = QStringLiteral("  现金  "),
        .openingBalance = Money::fromCents(12345),
    });

    QVERIFY(std::holds_alternative<Account>(result));
    const auto &account = std::get<Account>(result);
    QVERIFY(!account.id().isNull());
    QCOMPARE(account.name(), QStringLiteral("现金"));
    QVERIFY(account.openingBalance() == Money::fromCents(12345));
    QVERIFY(!account.isArchived());

    const auto stored = repository.findById(account.id());
    QVERIFY(stored.has_value());
    QCOMPARE(stored->name(), QStringLiteral("现金"));
}

void CreateAccountTest::rejectsInvalidInputWithoutSaving()
{
    InMemoryAccountRepository repository;
    CreateAccount createAccount(repository);

    const auto result = createAccount.execute(CreateAccountRequest{
        .name = QStringLiteral("   "),
        .openingBalance = Money::fromCents(0),
    });

    QVERIFY(std::holds_alternative<CreateAccountError>(result));
    QVERIFY(std::get<CreateAccountError>(result)
            == CreateAccountError::InvalidInput);
    QVERIFY(repository.findAll().empty());
}

void CreateAccountTest::reportsStorageFailure()
{
    InMemoryAccountRepository repository;
    repository.setSaveShouldSucceed(false);
    CreateAccount createAccount(repository);

    const auto result = createAccount.execute(CreateAccountRequest{
        .name = QStringLiteral("现金"),
        .openingBalance = Money::fromCents(0),
    });

    QVERIFY(std::holds_alternative<CreateAccountError>(result));
    QVERIFY(std::get<CreateAccountError>(result)
            == CreateAccountError::StorageFailure);
    QVERIFY(repository.findAll().empty());
}

void CreateAccountTest::generatesDifferentIds()
{
    InMemoryAccountRepository repository;
    CreateAccount createAccount(repository);

    const auto first = createAccount.execute(CreateAccountRequest{
        .name = QStringLiteral("现金"),
    });
    const auto second = createAccount.execute(CreateAccountRequest{
        .name = QStringLiteral("银行卡"),
    });

    QVERIFY(std::holds_alternative<Account>(first));
    QVERIFY(std::holds_alternative<Account>(second));
    QVERIFY(std::get<Account>(first).id() != std::get<Account>(second).id());
    QCOMPARE(repository.findAll().size(), std::size_t{2});
}

QTEST_APPLESS_MAIN(CreateAccountTest)

#include "CreateAccountTest.moc"
```

这里验证的不只是 Account 自身规则，而是整个用例流程：创建实体、保存、错误分支
以及 UUID 生成。

## 10. 创建第六个测试目标

在 `tests/unit/accounting/CMakeLists.txt` 末尾加入：

```cmake
add_executable(StuKitCreateAccountTest
    CreateAccountTest.cpp
    support/InMemoryAccountRepository.h
)

target_link_libraries(StuKitCreateAccountTest
    PRIVATE
        Qt6::Test
        StuKitAccountingApplication
)

add_test(
    NAME accounting.create_account
    COMMAND StuKitCreateAccountTest
)
```

Application 以 `PUBLIC` 方式链接 Domain，因此测试不需要重复写
`StuKitAccountingDomain`。

## 11. 当前架构关系

```text
StuKitCreateAccountTest
  -> CreateAccount
       -> IAccountRepository
       -> Account

InMemoryAccountRepository
  -> IAccountRepository
```

CreateAccount 测试注入内存实现。未来应用启动时会在 Composition Root 注入 SQLite
实现，用例代码本身不改变。

## 12. 构建与测试

在 Qt Creator 中：

1. 运行 CMake。
2. 检查 `StuKitAccountingApplication` 和 `StuKitCreateAccountTest` 目标是否出现。
3. 构建整个项目。
4. 运行全部测试。
5. 启动 StuKit，确认原有界面正常。

预期六组测试全部通过：

```text
accounting.money                 Passed
accounting.transaction           Passed
accounting.account               Passed
accounting.category              Passed
accounting.account_repository    Passed
accounting.create_account        Passed
```

## 13. 常见错误

### CMake 找不到 StuKitAccountingApplication

确认根 CMakeLists.txt 在测试子目录之前加入：

```cmake
add_subdirectory(src/features/accounting/application)
```

并重新运行 CMake。

### CreateAccount.h 找不到

确认 Application 目标使用 `target_include_directories(... PUBLIC ...)`，测试目标链接
了 `StuKitAccountingApplication`。

### std::variant、holds_alternative 或 get 未定义

确认使用这些类型的头文件和测试文件包含 `<variant>`。

### 测试报告 bad_variant_access

调用 `std::get<T>` 前先用 `std::holds_alternative<T>` 确认当前保存的类型。如果前一
条 QVERIFY 失败，先修复用例返回结果，不要继续强取另一种类型。

### StorageFailure 测试仍然返回成功

确认 `save` 在修改 `m_accounts` 之前检查 `m_saveShouldSucceed`，并且测试在构造用例
前调用了 `repository.setSaveShouldSucceed(false)`。

### 原来的 Repository 测试失败

确认 `m_saveShouldSucceed` 默认值是 `true`。新增失败模拟能力不能改变已有测试的
默认行为。

### Application 被加入 QML 模块

删除根 `STUKIT_QML_FILES` 中自动出现的 CreateAccount 或子目录 CMake 文件。C++
Application 通过普通 CMake 静态库管理，不属于 `qt_add_qml_module`。

如果错误显示编译 `StuKit` 目标中的 `CreateAccount.cpp` 时找不到
`IAccountRepository.h`，也应先检查这一项。问题不是头文件不存在，而是 Application
源码被错误地交给了没有 Domain 包含路径的 QML 应用目标编译。

### Createaccount 与 CreateAccount 大小写不一致

文件名、Application CMakeLists.txt 和 `#include` 统一使用 `CreateAccount.h` 与
`CreateAccount.cpp`。Windows 文件系统通常不区分大小写，错误可能暂时被隐藏；
Android/Linux 构建环境通常区分大小写，迁移时会直接报告文件不存在。

## 14. 本课验收清单

- [ ] 存在独立的 StuKitAccountingApplication 静态库。
- [ ] Application 只链接 StuKitAccountingDomain。
- [ ] Domain 没有反向链接 Application。
- [ ] CreateAccount 通过构造函数接收 IAccountRepository。
- [ ] 有效请求会生成非空 UUID、创建 Account 并保存。
- [ ] 无效输入返回 InvalidInput，且不访问存储写入。
- [ ] 保存失败返回 StorageFailure。
- [ ] 两次创建产生不同 ID。
- [ ] 第九课 Repository 测试仍然通过。
- [ ] 六组测试全部通过。
- [ ] StuKit 应用仍能正常运行。
- [ ] 根 QML 模块没有混入 Application 或测试源码。
- [ ] Git 状态中没有构建产物。

## 15. 完成后反馈

完成后先不要提交代码，请把以下信息发给我：

```text
CMake 配置：成功 / 失败
StuKitAccountingApplication：成功 / 失败
StuKitCreateAccountTest：成功 / 失败
六组测试结果：
StuKit 应用：正常 / 异常
应用或测试警告：
git diff --stat 输出：
```

下一步会在此基础上完善错误模型或进入账户查询用例，为后续 ViewModel 和 SQLite
实现做好准备。
