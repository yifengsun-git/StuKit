# 第六课：Transaction 实体与业务规则

## 本课目标

本课实现记账模块的第一个领域实体 `Transaction`，表示一笔收入或支出。

上一课的 Money 是值对象：金额相同就表示相同的值。Transaction 是实体：即使
两笔账单的金额、时间和备注完全相同，只要 ID 不同，它们仍是不同账单。

完成后，你应该理解：

- 实体和值对象的区别。
- 为什么实体需要稳定 ID。
- 如何通过工厂函数保证对象创建时有效。
- `std::optional` 如何表达创建成功或失败。
- 为什么领域实体不直接依赖数据库和 QML。
- 如何用数据驱动测试覆盖一组相似输入。

本课不实现数据库、编辑操作或界面表单。

## 1. 开始前保存第五课检查点

当前本地 main 比 origin/main 超前两个提交。先在 Qt Creator 中确认第五课仍能
构建和通过测试，然后推送：

```powershell
git push origin main
```

推送不会包含本课尚未提交的文档和代码，只会推送已有提交。

## 2. Transaction 的字段

一笔账单包含：

```text
id          账单自身 ID
type        Income 或 Expense
amount      金额，必须大于零
accountId   所属账户 ID
categoryId  所属分类 ID
occurredAt  实际发生时间
note        可选备注
```

ID 使用 QUuid，为以后导入、备份和多设备同步预留空间。

收入或支出由 TransactionType 明确表示，不通过金额正负判断。账单 amount 必须
始终大于零：

```text
Expense + 1000 分 = 支出 10.00 元
Income  + 1000 分 = 收入 10.00 元
```

这样可以避免同时使用“负金额”和“支出类型”表达同一含义。

## 3. 创建失败如何表示

本课使用：

```cpp
std::optional<Transaction>
```

- 包含 Transaction：创建成功。
- `std::nullopt`：输入违反业务规则。

`optional` 暂时不包含具体错误原因。后续接入表单时，我们会增加可向用户展示的
验证结果。本课先集中建立领域边界。

## 4. 本课新增文件

```text
src/features/accounting/domain/
  Transaction.h
  Transaction.cpp

tests/unit/accounting/
  TransactionTest.cpp
```

同时修改两个现有 CMakeLists.txt：

```text
src/features/accounting/domain/CMakeLists.txt
tests/unit/accounting/CMakeLists.txt
```

如果 Qt Creator 自动把这些 C++ 文件加入根目录的 `qt_add_qml_module`，必须
删除自动添加内容。它们分别属于领域库和测试目标，不属于 QML 模块。

## 5. 创建 Transaction 头文件

创建 `src/features/accounting/domain/Transaction.h`：

```cpp
#pragma once

#include "Money.h"

#include <QDateTime>
#include <QString>
#include <QUuid>

#include <optional>

namespace stukit::accounting {

enum class TransactionType
{
    Income,
    Expense
};

struct TransactionData
{
    QUuid id;
    TransactionType type{TransactionType::Expense};
    Money amount{Money::fromCents(0)};
    QUuid accountId;
    QUuid categoryId;
    QDateTime occurredAt;
    QString note;
};

class Transaction final
{
public:
    static std::optional<Transaction> create(TransactionData data);

    [[nodiscard]] const QUuid &id() const noexcept;
    [[nodiscard]] TransactionType type() const noexcept;
    [[nodiscard]] Money amount() const noexcept;
    [[nodiscard]] const QUuid &accountId() const noexcept;
    [[nodiscard]] const QUuid &categoryId() const noexcept;
    [[nodiscard]] const QDateTime &occurredAt() const noexcept;
    [[nodiscard]] const QString &note() const noexcept;

private:
    explicit Transaction(TransactionData data);

    TransactionData m_data;
};

} // namespace stukit::accounting
```

`TransactionData` 是创建实体时使用的参数对象，避免工厂函数包含七个难以辨认
的连续参数。它可以临时修改，但创建成功后的 Transaction 只通过只读 getter
暴露数据。

TransactionData 给 amount 一个零值默认值。零金额无法通过工厂验证，因此忘记
赋值时会安全地创建失败。

## 6. 实现 Transaction

创建 `src/features/accounting/domain/Transaction.cpp`：

```cpp
#include "Transaction.h"

#include <utility>

namespace stukit::accounting {

std::optional<Transaction> Transaction::create(TransactionData data)
{
    if (data.id.isNull()
        || data.accountId.isNull()
        || data.categoryId.isNull()
        || !data.amount.isPositive()
        || !data.occurredAt.isValid()) {
        return std::nullopt;
    }

    data.note = data.note.trimmed();

    return Transaction(std::move(data));
}

const QUuid &Transaction::id() const noexcept
{
    return m_data.id;
}

TransactionType Transaction::type() const noexcept
{
    return m_data.type;
}

Money Transaction::amount() const noexcept
{
    return m_data.amount;
}

const QUuid &Transaction::accountId() const noexcept
{
    return m_data.accountId;
}

const QUuid &Transaction::categoryId() const noexcept
{
    return m_data.categoryId;
}

const QDateTime &Transaction::occurredAt() const noexcept
{
    return m_data.occurredAt;
}

const QString &Transaction::note() const noexcept
{
    return m_data.note;
}

Transaction::Transaction(TransactionData data)
    : m_data(std::move(data))
{
}

} // namespace stukit::accounting
```

工厂函数集中执行创建规则：

- 三个 ID 都不能是空 UUID。
- 金额必须大于零。
- 发生时间必须有效。
- 备注自动去除首尾空格。

本课不禁止未来时间。是否允许录入未来账单属于产品规则，而且依赖“当前时间”，
需要以后通过可测试的 Clock 服务判断，不能在实体中随意读取系统时钟。

## 7. 将 Transaction 加入领域库

修改 `src/features/accounting/domain/CMakeLists.txt`：

```cmake
add_library(StuKitAccountingDomain STATIC
    Money.cpp
    Money.h
    Transaction.cpp
    Transaction.h
)
```

其他 include、link 和 compile features 配置保持不变。

## 8. 创建测试文件

创建 `tests/unit/accounting/TransactionTest.cpp`：

```cpp
#include <QtTest>

#include "Transaction.h"

using stukit::accounting::Money;
using stukit::accounting::Transaction;
using stukit::accounting::TransactionData;
using stukit::accounting::TransactionType;

namespace {

TransactionData validTransactionData()
{
    TransactionData data;
    data.id = QUuid::fromString(
        QStringLiteral("{11111111-1111-1111-1111-111111111111}"));
    data.type = TransactionType::Expense;
    data.amount = Money::fromCents(1299);
    data.accountId = QUuid::fromString(
        QStringLiteral("{22222222-2222-2222-2222-222222222222}"));
    data.categoryId = QUuid::fromString(
        QStringLiteral("{33333333-3333-3333-3333-333333333333}"));
    data.occurredAt = QDateTime::fromString(
        QStringLiteral("2026-10-07T12:30:00Z"), Qt::ISODate);
    data.note = QStringLiteral("  lunch  ");
    return data;
}

} // namespace

class TransactionTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsValidTransaction();
    void createsIncomeTransaction();
    void rejectsNullTransactionId();
    void rejectsNullAccountId();
    void rejectsNullCategoryId();
    void rejectsNonPositiveAmount_data();
    void rejectsNonPositiveAmount();
    void rejectsInvalidOccurredAt();
};

void TransactionTest::createsValidTransaction()
{
    const TransactionData data = validTransactionData();
    const std::optional<Transaction> transaction = Transaction::create(data);

    QVERIFY(transaction.has_value());
    QCOMPARE(transaction->id(), data.id);
    QVERIFY(transaction->type() == TransactionType::Expense);
    QVERIFY(transaction->amount() == data.amount);
    QCOMPARE(transaction->accountId(), data.accountId);
    QCOMPARE(transaction->categoryId(), data.categoryId);
    QCOMPARE(transaction->occurredAt(), data.occurredAt);
    QCOMPARE(transaction->note(), QStringLiteral("lunch"));
}

void TransactionTest::createsIncomeTransaction()
{
    TransactionData data = validTransactionData();
    data.type = TransactionType::Income;

    const std::optional<Transaction> transaction = Transaction::create(data);

    QVERIFY(transaction.has_value());
    QVERIFY(transaction->type() == TransactionType::Income);
}

void TransactionTest::rejectsNullTransactionId()
{
    TransactionData data = validTransactionData();
    data.id = QUuid{};

    QVERIFY(!Transaction::create(data).has_value());
}

void TransactionTest::rejectsNullAccountId()
{
    TransactionData data = validTransactionData();
    data.accountId = QUuid{};

    QVERIFY(!Transaction::create(data).has_value());
}

void TransactionTest::rejectsNullCategoryId()
{
    TransactionData data = validTransactionData();
    data.categoryId = QUuid{};

    QVERIFY(!Transaction::create(data).has_value());
}

void TransactionTest::rejectsNonPositiveAmount_data()
{
    QTest::addColumn<qint64>("cents");

    QTest::newRow("zero") << qint64{0};
    QTest::newRow("negative") << qint64{-1};
}

void TransactionTest::rejectsNonPositiveAmount()
{
    QFETCH(qint64, cents);

    TransactionData data = validTransactionData();
    data.amount = Money::fromCents(cents);

    QVERIFY(!Transaction::create(data).has_value());
}

void TransactionTest::rejectsInvalidOccurredAt()
{
    TransactionData data = validTransactionData();
    data.occurredAt = QDateTime{};

    QVERIFY(!Transaction::create(data).has_value());
}

QTEST_APPLESS_MAIN(TransactionTest)

#include "TransactionTest.moc"
```

`rejectsNonPositiveAmount_data` 为测试提供两行数据。Qt Test 会分别执行 zero 和
negative，因此一个测试行为可以覆盖多个输入而不复制整个测试函数。

## 9. 注册 Transaction 测试目标

在 `tests/unit/accounting/CMakeLists.txt` 原有 Money 测试配置后增加：

```cmake
add_executable(StuKitTransactionTest
    TransactionTest.cpp
)

target_link_libraries(StuKitTransactionTest
    PRIVATE
        Qt6::Test
        StuKitAccountingDomain
)

add_test(
    NAME accounting.transaction
    COMMAND StuKitTransactionTest
)
```

不要复制或删除 Money 测试配置。完成后应同时存在：

```text
accounting.money
accounting.transaction
```

## 10. 重新配置、构建和测试

新增文件并修改 CMake 后：

1. 在 Qt Creator 中运行 CMake。
2. 构建 `all` 或分别构建两个测试目标。
3. 在 Tests 面板运行全部测试。
4. 最后运行 StuKit，确认现有界面没有受到影响。

预期 CTest 结果：

```text
100% tests passed, 0 tests failed out of 2
```

如果从普通 PowerShell 运行 CTest，需要让 Qt 6.10.1 MSVC 的 `bin` 目录出现在
当前终端 PATH 中。使用 Qt Creator Tests 面板可以直接继承 Kit 环境。

## 11. 理解当前架构

本课结束后的关系：

```text
StuKitMoneyTest -----------+
                           |
StuKitTransactionTest -----+--> StuKitAccountingDomain --> Qt6::Core

StuKit 应用 --> QML 界面
```

应用暂时没有链接记账领域库，这不是遗漏。下一阶段会添加 Application 用例和
ViewModel，在那时才建立界面到后端的受控调用路径。

## 12. 常见错误

### Transaction.h 找不到 Money.h

两个文件必须位于同一领域目录，并由 StuKitAccountingDomain 管理。不要将
Transaction 单独加入 StuKit QML 模块。

### std::optional 未定义

检查 Transaction.h 是否包含 `<optional>`。

### std::move 未定义

检查 Transaction.cpp 是否包含 `<utility>`。

### QCOMPARE 无法比较 TransactionType 或 Money

本课对 TransactionType 和 Money 使用 `QVERIFY(left == right)`，不要求为自定义
类型额外实现 QDebug 输出。

### 只发现 accounting.money

检查 tests/unit/accounting/CMakeLists.txt 是否注册了 `accounting.transaction`，
并重新运行 CMake。

### StuKitTransactionTest 目标不存在

每个使用 `QTEST_APPLESS_MAIN` 的测试类都必须拥有独立的可执行目标。不要把
`MoneyTest.cpp` 和 `TransactionTest.cpp` 同时放入 `StuKitMoneyTest`。应分别创建
`StuKitMoneyTest` 与 `StuKitTransactionTest`，再为它们各自调用一次 `add_test`。

### Qt Creator 自动修改根 CMakeLists.txt

删除 `qt_add_qml_module` 中自动出现的 Transaction、测试或 CMake 文件条目。
根 QML 模块仍然只通过 `${STUKIT_QML_FILES}` 注册 QML 文件。

### MSVC 报 fatal error C1902

这通常不是 Transaction 代码错误，而是 MSVC 编译器与 PDB 工具版本不一致。
先通过 Visual Studio Installer 更新或修复当前 MSVC 工具链，重启 Qt Creator 后
重新运行 CMake。不要从其他目录手工复制 `mspdb*.dll` 或 `mspdbsrv.exe`。

## 13. 本课验收清单

- [ ] TransactionType 明确区分收入和支出。
- [ ] Transaction 使用 QUuid 表示自身、账户和分类 ID。
- [ ] amount 必须大于零。
- [ ] occurredAt 必须有效。
- [ ] 备注会自动去除首尾空格。
- [ ] Transaction 不能绕过工厂函数直接创建。
- [ ] Transaction 属于 StuKitAccountingDomain。
- [ ] Money 与 Transaction 两组测试全部通过。
- [ ] StuKit 应用仍能正常启动和切换主题。
- [ ] 根 QML 模块没有混入领域或测试源码。
- [ ] Git 状态中没有构建产物。

## 14. 完成后反馈

请提供：

```text
CMake 配置：成功 / 失败
StuKitAccountingDomain：成功 / 失败
StuKitTransactionTest：成功 / 失败
accounting.money：通过 / 失败
accounting.transaction：通过 / 失败
全部测试结果：
StuKit 应用：正常 / 异常
应用或测试警告：
git diff --stat 输出：
```

暂时不要提交代码。验收通过后，我们再整理格式、更新文档并拆分提交。
