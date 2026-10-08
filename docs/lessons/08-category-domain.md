# 第八课：Category 分类实体与共享领域类型

## 本课目标

本课实现记账模块的 `Category` 分类实体，并把收入、支出枚举
`TransactionType` 从 `Transaction.h` 提取到独立头文件。

完成后，你应该理解：

- 为什么两个领域对象共享的概念不应重复定义。
- 如何进行不改变行为的小范围重构。
- 如何用已有测试保护重构过程。
- 哪些规则属于单个实体，哪些规则需要留给应用用例。
- 为什么分类也采用归档，而不是直接删除。

本课不连接数据库，不实现分类管理页面，也不校验分类名称是否重复。

## 1. 为什么先提取 TransactionType

Transaction 已经使用：

```cpp
enum class TransactionType
{
    Income,
    Expense
};
```

Category 也必须区分收入分类和支出分类。例如“工资”只能用于收入，“餐饮”只能
用于支出。

不应该在 Category 中再定义一个内容相同的枚举，否则两个类型虽然名字和值相同，
在 C++ 中仍然是不同类型。也不建议让 `Category.h` 为了一个枚举而包含完整的
`Transaction.h`。

本课把这个共享概念移动到：

```text
TransactionType.h
```

Transaction 和 Category 都包含它。这个类型仍然只属于记账 Domain，不进入
`shared/`，因为课表模块并不需要收入和支出的概念。

## 2. Category 的字段和规则

Category 保存：

```text
id        分类的稳定 ID
name      展示名称
type      Income 或 Expense
archived  是否已经归档
```

名称规则与 Account 保持一致：

- 去除首尾空白。
- 处理后不能为空。
- 最长 40 个字符。

已经被账单引用的分类不能随意物理删除，因此也采用归档状态。分类名称是否重复
需要查询其他分类，单个 Category 无法独立判断，后续由应用用例和 Repository
协作完成。

Transaction 的类型是否与 Category 的类型一致，同样是跨实体规则，将在新增账单
用例中检查，不塞进任何一个实体的 `create` 函数。

## 3. 本课文件变化

新增：

```text
src/features/accounting/domain/TransactionType.h
src/features/accounting/domain/Category.h
src/features/accounting/domain/Category.cpp
tests/unit/accounting/CategoryTest.cpp
```

修改：

```text
src/features/accounting/domain/Transaction.h
src/features/accounting/domain/CMakeLists.txt
tests/unit/accounting/CMakeLists.txt
```

## 4. 提取 TransactionType

创建 `src/features/accounting/domain/TransactionType.h`：

```cpp
#pragma once

namespace stukit::accounting {

enum class TransactionType
{
    Income,
    Expense
};

} // namespace stukit::accounting
```

然后修改 `Transaction.h`：

1. 在 `#include "Money.h"` 后加入：

```cpp
#include "TransactionType.h"
```

2. 从 `Transaction.h` 中删除原来的整段 `TransactionType` 枚举定义。

修改后 Transaction 的对外行为没有变化，只是枚举的定义位置变了。完成这一步
后可以先构建并运行原有三组测试。如果 `accounting.transaction` 仍然通过，说明
这次重构没有破坏原有功能。

## 5. 创建 Category.h

创建 `src/features/accounting/domain/Category.h`：

```cpp
#pragma once

#include "TransactionType.h"

#include <QString>
#include <QUuid>

#include <optional>

namespace stukit::accounting {

struct CategoryData
{
    QUuid id;
    QString name;
    TransactionType type{TransactionType::Expense};
    bool archived{false};
};

class Category final
{
public:
    static constexpr qsizetype MaximumNameLength = 40;

    static std::optional<Category> create(CategoryData data);

    [[nodiscard]] const QUuid &id() const noexcept;
    [[nodiscard]] const QString &name() const noexcept;
    [[nodiscard]] TransactionType type() const noexcept;
    [[nodiscard]] bool isArchived() const noexcept;

private:
    explicit Category(CategoryData data);

    CategoryData m_data;
};

} // namespace stukit::accounting
```

默认类型使用 `Expense` 只是为了让结构具有安全的初始值，不代表业务中不能创建
收入分类。调用者创建收入分类时必须明确传入 `TransactionType::Income`。

## 6. 创建 Category.cpp

创建 `src/features/accounting/domain/Category.cpp`：

```cpp
#include "Category.h"

#include <utility>

namespace stukit::accounting {

std::optional<Category> Category::create(CategoryData data)
{
    data.name = data.name.trimmed();

    if (data.id.isNull()
        || data.name.isEmpty()
        || data.name.size() > MaximumNameLength) {
        return std::nullopt;
    }

    return Category(std::move(data));
}

Category::Category(CategoryData data)
    : m_data(std::move(data))
{
}

const QUuid &Category::id() const noexcept
{
    return m_data.id;
}

const QString &Category::name() const noexcept
{
    return m_data.name;
}

TransactionType Category::type() const noexcept
{
    return m_data.type;
}

bool Category::isArchived() const noexcept
{
    return m_data.archived;
}

} // namespace stukit::accounting
```

Category 与 Account 的名称规则相似，但暂时不要为了减少几行代码就创建抽象基类
或通用实体。只有重复模式稳定并且确实被多个模块需要时，才考虑抽取公共能力。

## 7. 更新领域库

在 `src/features/accounting/domain/CMakeLists.txt` 的
`StuKitAccountingDomain` 源文件列表中加入：

```cmake
    TransactionType.h
    Category.cpp
    Category.h
```

`TransactionType.h` 虽然没有对应的 `.cpp`，仍然可以列入目标，方便 IDE 展示和
代码审查。

不要把这些文件加入根目录的 `STUKIT_QML_FILES`。

## 8. 创建 CategoryTest.cpp

创建 `tests/unit/accounting/CategoryTest.cpp`：

```cpp
#include "Category.h"

#include <QTest>

using namespace stukit::accounting;

namespace {

CategoryData validCategoryData()
{
    return CategoryData{
        .id = QUuid::createUuid(),
        .name = QStringLiteral("餐饮"),
        .type = TransactionType::Expense,
        .archived = false,
    };
}

} // namespace

class CategoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsExpenseCategory();
    void createsIncomeCategory();
    void rejectsNullId();
    void rejectsInvalidName_data();
    void rejectsInvalidName();
    void acceptsMaximumLengthName();
    void preservesArchivedState();
};

void CategoryTest::createsExpenseCategory()
{
    auto data = validCategoryData();
    data.name = QStringLiteral("  餐饮  ");

    const auto category = Category::create(data);

    QVERIFY(category.has_value());
    QCOMPARE(category->id(), data.id);
    QCOMPARE(category->name(), QStringLiteral("餐饮"));
    QVERIFY(category->type() == TransactionType::Expense);
    QVERIFY(!category->isArchived());
}

void CategoryTest::createsIncomeCategory()
{
    auto data = validCategoryData();
    data.name = QStringLiteral("工资");
    data.type = TransactionType::Income;

    const auto category = Category::create(data);

    QVERIFY(category.has_value());
    QVERIFY(category->type() == TransactionType::Income);
}

void CategoryTest::rejectsNullId()
{
    auto data = validCategoryData();
    data.id = QUuid{};

    QVERIFY(!Category::create(data).has_value());
}

void CategoryTest::rejectsInvalidName_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("empty") << QString{};
    QTest::newRow("only whitespace") << QStringLiteral("   ");
    QTest::newRow("too long")
        << QString(Category::MaximumNameLength + 1, QLatin1Char('a'));
}

void CategoryTest::rejectsInvalidName()
{
    QFETCH(QString, name);

    auto data = validCategoryData();
    data.name = name;

    QVERIFY(!Category::create(data).has_value());
}

void CategoryTest::acceptsMaximumLengthName()
{
    auto data = validCategoryData();
    data.name = QString(Category::MaximumNameLength, QLatin1Char('a'));

    const auto category = Category::create(data);

    QVERIFY(category.has_value());
    QCOMPARE(category->name().size(), Category::MaximumNameLength);
}

void CategoryTest::preservesArchivedState()
{
    auto data = validCategoryData();
    data.archived = true;

    const auto category = Category::create(data);

    QVERIFY(category.has_value());
    QVERIFY(category->isArchived());
}

QTEST_APPLESS_MAIN(CategoryTest)

#include "CategoryTest.moc"
```

`acceptsMaximumLengthName()` 是边界测试：规则是“长度大于 40 才失败”，所以长度
刚好等于 40 必须成功。

## 9. 创建测试目标

在 `tests/unit/accounting/CMakeLists.txt` 末尾加入：

```cmake
add_executable(StuKitCategoryTest
    CategoryTest.cpp
)

target_link_libraries(StuKitCategoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingDomain
)

add_test(
    NAME accounting.category
    COMMAND StuKitCategoryTest
)
```

`CategoryTest.cpp` 只能出现在 `StuKitCategoryTest` 中。不要把它加入已有的 Money、
Transaction 或 Account 测试目标。

## 10. 构建和测试顺序

建议按下面的顺序验证：

1. 提取 TransactionType 后，先运行 CMake、构建并执行原有三组测试。
2. 完成 Category 后，再运行 CMake、构建并执行全部四组测试。
3. 启动 StuKit，检查导航、占位页面和主题切换。

最终预期：

```text
accounting.money          Passed
accounting.transaction    Passed
accounting.account        Passed
accounting.category       Passed
```

## 11. 常见错误

### TransactionType 重复定义

创建 `TransactionType.h` 后，必须删除 `Transaction.h` 中原来的枚举定义。
`Transaction.h` 和 `Category.h` 都只包含 `TransactionType.h`。

### 枚举行末误输入反斜杠

`enum class TransactionType` 行末不能带 `\`。C/C++ 预处理器会把反斜杠和紧随
其后的换行当作续行，因此代码可能碰巧仍能编译，却会形成难以发现的格式问题。

### TransactionType 未定义

确认使用该类型的头文件直接包含了 `TransactionType.h`。不要依赖其他头文件碰巧
间接包含它。

### 修改头文件后出现旧错误

先运行 CMake，再重新构建。如果错误仍然引用已经删除的代码，可以在 Qt Creator
中执行“重新构建”，不需要删除源文件。

### 链接器报告 main 重复定义

检查 `CategoryTest.cpp` 是否被加入多个测试目标。每个包含
`QTEST_APPLESS_MAIN` 的测试文件都必须拥有独立的 `add_executable`。

### QCOMPARE 无法显示 TransactionType

本课使用：

```cpp
QVERIFY(category->type() == TransactionType::Expense);
```

这样不要求为枚举额外提供调试输出函数。

## 12. 本课验收清单

- [ ] TransactionType 已移动到独立头文件。
- [ ] 原有 Transaction 测试仍然通过。
- [ ] Category 拥有非空 UUID、名称、类型和归档状态。
- [ ] 分类名称会去除首尾空白。
- [ ] 空名称、纯空白名称和超过 40 个字符的名称会被拒绝。
- [ ] 长度刚好为 40 的名称能够创建成功。
- [ ] 收入和支出分类都能创建。
- [ ] Category 属于 StuKitAccountingDomain。
- [ ] 四组领域测试全部通过。
- [ ] StuKit 应用仍能正常运行。
- [ ] 根 QML 模块没有混入领域或测试源码。
- [ ] Git 状态中没有构建产物。

## 13. 完成后反馈

完成后先不要提交代码，请把以下信息发给我：

```text
CMake 配置：成功 / 失败
重构后的原有三组测试：通过 / 失败
StuKitCategoryTest：成功 / 失败
四组测试结果：
StuKit 应用：正常 / 异常
应用或测试警告：
git diff --stat 输出：
```

我会先检查重构是否完整、测试目标是否独立，再帮助你整理本课提交。
