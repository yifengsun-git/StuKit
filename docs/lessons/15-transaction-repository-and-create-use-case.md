# 第十五课：Transaction Repository 与 CreateTransaction 用例

## 本课目标

本课不写 SQL，而是完成第一条真正连接多个业务对象的记账流程：新增账单。

完成后，你应该理解：

- 如何为 Transaction 定义 Repository 接口。
- 如何使用内存 Repository 测试 Application 用例。
- 为什么新增账单前必须查询账户和分类。
- 如何拒绝已归档账户、已归档分类和类型不匹配的分类。
- 如何为不同失败原因设计明确的错误枚举。
- 为什么跨实体规则属于 Application，而不属于 QML 或单个实体。

本课完成后，自动测试目标从 11 个增加到 13 个。第十六课再由 Codex 负责迁移
版本 3、`transactions` 表和 SQLite Transaction Repository。

## 1. CreateTransaction 要处理什么

单个 `Transaction::create()` 只能检查自身字段：

- UUID 不能为空。
- 金额必须大于零。
- 发生时间必须有效。
- 备注需要去除首尾空白。

但创建账单还需要跨实体检查：

```text
账户是否存在？
账户是否已经归档？
分类是否存在？
分类是否已经归档？
分类是收入还是支出？是否与账单类型一致？
```

这些检查需要 Repository 查询其他实体，因此由 `CreateTransaction` 用例组织。

## 2. 调用关系

```text
CreateTransactionRequest
  -> CreateTransaction
       -> Transaction::create()
       -> IAccountRepository::findById()
       -> ICategoryRepository::findById()
       -> ITransactionRepository::save()
```

用例依赖三个接口，但仍然不知道数据来自内存还是 SQLite。

## 3. 文件变化

新增：

```text
src/features/accounting/domain/ITransactionRepository.h
src/features/accounting/application/CreateTransaction.h
src/features/accounting/application/CreateTransaction.cpp
tests/unit/accounting/support/InMemoryTransactionRepository.h
tests/unit/accounting/TransactionRepositoryTest.cpp
tests/unit/accounting/CreateTransactionTest.cpp
```

修改：

```text
src/features/accounting/domain/CMakeLists.txt
src/features/accounting/application/CMakeLists.txt
tests/unit/accounting/CMakeLists.txt
```

## 4. 定义 ITransactionRepository

创建 `src/features/accounting/domain/ITransactionRepository.h`：

```cpp
#pragma once

#include "Transaction.h"

#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::accounting {

class ITransactionRepository
{
public:
    virtual ~ITransactionRepository() = default;

    [[nodiscard]] virtual bool save(const Transaction &transaction) = 0;
    [[nodiscard]] virtual std::optional<Transaction> findById(
        const QUuid &id) const = 0;
    [[nodiscard]] virtual std::vector<Transaction> findAll() const = 0;
};

} // namespace stukit::accounting
```

将它加入 Domain 的 CMake 目标：

```cmake
    IAccountRepository.h
    ICategoryRepository.h
    ITransactionRepository.h
```

以后会增加按月份查询，但本课先保持最小接口，避免为尚未实现的页面提前设计复杂
查询参数。

## 5. 创建内存 Transaction Repository

创建 `tests/unit/accounting/support/InMemoryTransactionRepository.h`。实现方式与前两
个内存 Repository 相同：

```cpp
#pragma once

#include "ITransactionRepository.h"

#include <algorithm>

namespace stukit::accounting::testing {

class InMemoryTransactionRepository final : public ITransactionRepository
{
public:
    [[nodiscard]] bool save(const Transaction &transaction) override
    {
        if (!m_saveShouldSucceed) {
            return false;
        }

        const auto existing = std::find_if(
            m_transactions.begin(),
            m_transactions.end(),
            [&transaction](const Transaction &stored) {
                return stored.id() == transaction.id();
            });

        if (existing == m_transactions.end()) {
            m_transactions.push_back(transaction);
        } else {
            *existing = transaction;
        }

        return true;
    }

    [[nodiscard]] std::optional<Transaction> findById(
        const QUuid &id) const override
    {
        const auto transaction = std::find_if(
            m_transactions.cbegin(),
            m_transactions.cend(),
            [&id](const Transaction &stored) {
                return stored.id() == id;
            });

        if (transaction == m_transactions.cend()) {
            return std::nullopt;
        }

        return *transaction;
    }

    [[nodiscard]] std::vector<Transaction> findAll() const override
    {
        return m_transactions;
    }

    void setSaveShouldSucceed(bool shouldSucceed) noexcept
    {
        m_saveShouldSucceed = shouldSucceed;
    }

private:
    std::vector<Transaction> m_transactions;
    bool m_saveShouldSucceed{true};
};

} // namespace stukit::accounting::testing
```

## 6. 为 Repository 契约编写测试

创建 `tests/unit/accounting/TransactionRepositoryTest.cpp`，至少覆盖：

```text
savesAndFindsTransaction
returnsNulloptForMissingTransaction
updatesTransactionWithSameId
returnsAllTransactions
```

可以建立辅助函数：

```cpp
Transaction makeTransaction(
    const QUuid &id,
    Money amount = Money::fromCents(1000))
{
    const auto transaction = Transaction::create(TransactionData{
        .id = id,
        .type = TransactionType::Expense,
        .amount = amount,
        .accountId = QUuid::createUuid(),
        .categoryId = QUuid::createUuid(),
        .occurredAt = QDateTime::fromString(
            QStringLiteral("2026-10-10T12:00:00Z"), Qt::ISODate),
        .note = QStringLiteral("测试"),
    });

    Q_ASSERT(transaction.has_value());
    return *transaction;
}
```

测试结构可以参考 `AccountRepositoryTest.cpp`，但不要把两个测试文件放进同一个
可执行目标。

## 7. 定义 CreateTransaction 请求和错误

创建 `src/features/accounting/application/CreateTransaction.h`：

```cpp
#pragma once

#include "IAccountRepository.h"
#include "ICategoryRepository.h"
#include "ITransactionRepository.h"

#include <QDateTime>
#include <QString>
#include <QUuid>

#include <variant>

namespace stukit::accounting {

struct CreateTransactionRequest
{
    TransactionType type{TransactionType::Expense};
    Money amount{Money::fromCents(0)};
    QUuid accountId;
    QUuid categoryId;
    QDateTime occurredAt;
    QString note;
};

enum class CreateTransactionError
{
    InvalidInput,
    AccountNotFound,
    AccountArchived,
    CategoryNotFound,
    CategoryArchived,
    CategoryTypeMismatch,
    StorageFailure
};

using CreateTransactionResult =
    std::variant<Transaction, CreateTransactionError>;

class CreateTransaction final
{
public:
    CreateTransaction(
        ITransactionRepository &transactionRepository,
        IAccountRepository &accountRepository,
        ICategoryRepository &categoryRepository);

    [[nodiscard]] CreateTransactionResult execute(
        const CreateTransactionRequest &request);

private:
    ITransactionRepository &m_transactionRepository;
    IAccountRepository &m_accountRepository;
    ICategoryRepository &m_categoryRepository;
};

} // namespace stukit::accounting
```

错误枚举不直接保存中文提示。Presentation 层以后负责将这些稳定错误类型转换成
用户可见文案。

## 8. 实现 CreateTransaction

创建 `CreateTransaction.cpp`：

```cpp
#include "CreateTransaction.h"

#include <QUuid>

namespace stukit::accounting {

CreateTransaction::CreateTransaction(
    ITransactionRepository &transactionRepository,
    IAccountRepository &accountRepository,
    ICategoryRepository &categoryRepository)
    : m_transactionRepository(transactionRepository)
    , m_accountRepository(accountRepository)
    , m_categoryRepository(categoryRepository)
{
}

CreateTransactionResult CreateTransaction::execute(
    const CreateTransactionRequest &request)
{
    const auto transaction = Transaction::create(TransactionData{
        .id = QUuid::createUuid(),
        .type = request.type,
        .amount = request.amount,
        .accountId = request.accountId,
        .categoryId = request.categoryId,
        .occurredAt = request.occurredAt,
        .note = request.note,
    });

    if (!transaction.has_value()) {
        return CreateTransactionError::InvalidInput;
    }

    const auto account = m_accountRepository.findById(request.accountId);
    if (!account.has_value()) {
        return CreateTransactionError::AccountNotFound;
    }
    if (account->isArchived()) {
        return CreateTransactionError::AccountArchived;
    }

    const auto category = m_categoryRepository.findById(request.categoryId);
    if (!category.has_value()) {
        return CreateTransactionError::CategoryNotFound;
    }
    if (category->isArchived()) {
        return CreateTransactionError::CategoryArchived;
    }
    if (category->type() != request.type) {
        return CreateTransactionError::CategoryTypeMismatch;
    }

    if (!m_transactionRepository.save(*transaction)) {
        return CreateTransactionError::StorageFailure;
    }

    return *transaction;
}

} // namespace stukit::accounting
```

顺序很重要：先检查请求能否形成有效 Transaction，再查询关联实体，最后保存。任何
一步失败都不能向 Transaction Repository 写入数据。

将两个文件加入 Application 的 CMake 目标。

## 9. CreateTransaction 测试场景

创建 `tests/unit/accounting/CreateTransactionTest.cpp`。测试夹具需要三个内存
Repository，并准备一个有效账户和一个有效分类。

至少覆盖：

```text
createsAndStoresValidTransaction
rejectsInvalidInputWithoutSaving
reportsMissingAccount
rejectsArchivedAccount
reportsMissingCategory
rejectsArchivedCategory
rejectsCategoryTypeMismatch
reportsStorageFailure
```

有效请求辅助函数可以写成：

```cpp
CreateTransactionRequest validRequest(
    const QUuid &accountId,
    const QUuid &categoryId)
{
    return CreateTransactionRequest{
        .type = TransactionType::Expense,
        .amount = Money::fromCents(2599),
        .accountId = accountId,
        .categoryId = categoryId,
        .occurredAt = QDateTime::fromString(
            QStringLiteral("2026-10-10T12:00:00Z"), Qt::ISODate),
        .note = QStringLiteral("  午餐  "),
    };
}
```

成功测试除了检查返回值，还要从 `InMemoryTransactionRepository` 重新查询，确认
备注已经清理、金额和关联 UUID 均正确保存。

测试归档分支时，应使用 `Account::create()` 或 `Category::create()` 创建
`archived = true` 的实体并保存到对应内存 Repository，不要为了测试修改实体私有
数据。

## 10. 注册两个测试目标

在 `tests/unit/accounting/CMakeLists.txt` 末尾分别创建：

```text
StuKitTransactionRepositoryTest -> accounting.transaction_repository
StuKitCreateTransactionTest     -> accounting.create_transaction
```

第一个链接 `StuKitAccountingDomain`，第二个链接
`StuKitAccountingApplication`。CreateTransaction 测试目标还要列出三个内存
Repository 头文件，方便 Qt Creator 展示项目结构。

不要把新测试源文件加入旧测试目标，也不要加入根 QML 模块。

## 11. 本课暂不解决的 Repository 查询错误

现有 `findById()` 使用 `std::optional`，因此 Application 暂时无法区分：

```text
确实没有这条记录
数据库查询本身失败
```

内存测试中不存在这个问题，但真实 SQLite 用例接入前需要设计统一查询结果类型。
这会作为后续错误模型课程处理。本课先保持现有接口一致，不临时引入另一套返回
方式。

## 12. 构建和验收

在 Qt Creator 中：

1. 运行 CMake。
2. 重新构建项目。
3. 运行全部测试。
4. 启动 StuKit 应用。
5. 执行 `git diff --check`。

预期新增测试：

```text
accounting.transaction_repository
accounting.create_transaction
```

预期总结果：

```text
13/13 tests passed
```

## 13. 常见错误

### `ITransactionRepository.h` 找不到

确认它已加入 Domain CMake 目标，并且 Application 继续链接
`StuKitAccountingDomain`。

### 构造 CreateTransaction 时参数不匹配

构造函数顺序固定为：Transaction Repository、Account Repository、Category
Repository。建议使用明确变量名，不要创建三个临时对象。

### 有效账单返回 CategoryTypeMismatch

检查请求的 `type` 与测试分类的 `type` 是否一致。餐饮通常是 Expense，工资通常是
Income。

### 归档测试没有进入预期分支

确认归档实体使用与请求相同的 UUID，并且已经保存到对应内存 Repository。

### 链接错误提示 main 重复定义

每个包含 `QTEST_APPLESS_MAIN` 的 `.cpp` 必须拥有独立 `add_executable()`。

## 14. 验收清单

- [x] `ITransactionRepository` 位于 Domain，且不依赖数据库。
- [x] 内存 Transaction Repository 支持保存、查询、更新和模拟失败。
- [x] `CreateTransaction` 位于 Application。
- [x] 无效金额、UUID 或时间返回 `InvalidInput`。
- [x] 不存在和已归档账户分别返回不同错误。
- [x] 不存在和已归档分类分别返回不同错误。
- [x] 分类类型与账单类型必须一致。
- [x] 任意校验失败都不会保存账单。
- [x] 保存失败返回 `StorageFailure`。
- [x] 成功结果保留金额、关联 UUID、时间和清理后的备注。
- [x] 两个新测试拥有独立 CMake 目标。
- [x] 13 组测试全部通过。
- [x] StuKit 应用仍能正常构建和启动。
- [x] `git diff --check` 没有输出。

## 15. 完成后反馈

完成后先不要提交，把以下信息发给我：

```text
CMake 配置：成功 / 失败
TransactionRepositoryTest：成功 / 失败
CreateTransactionTest：成功 / 失败
13 组测试结果：
StuKit 应用：正常 / 异常
完整错误或警告：
git diff --stat 输出：
```

我会负责审查、排错、文档更新和提交。第十六课的 SQLite Transaction
Repository 与迁移版本 3 继续由我实现。
