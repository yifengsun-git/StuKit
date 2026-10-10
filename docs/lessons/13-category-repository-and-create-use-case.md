# 第十三课：Category Repository 与 CreateCategory 用例

## 本课目标

本课不写 SQL。我们先在 Domain 和 Application 层完成“创建分类”的业务流程，
并使用内存 Repository 做快速单元测试。

完成后，你应该理解：

- 为什么 Repository 接口属于 Domain，而具体存储实现不属于 Domain。
- 如何为 Category 定义与 Account 相似的存储契约。
- 如何使用内存 Repository 隔离数据库进行测试。
- 如何由 Application 用例组织 UUID 生成、领域校验和保存流程。
- 为什么 CreateCategory 不应该创建 SQLite 连接或操作 QML 控件。
- 相似功能应复用架构思路，而不是让 Account 和 Category 共用一个大接口。

本课完成后，测试总数会从 8 组增加到 10 组。第十四课再由 Codex 负责数据库
迁移和 SQLite Category Repository，你可以以后再集中学习 SQL 部分。

## 1. 本课调用关系

```text
CreateCategoryRequest
  -> CreateCategory
       -> Category::create()
       -> ICategoryRepository::save()

单元测试中：
ICategoryRepository <- InMemoryCategoryRepository

未来正式运行时：
ICategoryRepository <- SqliteCategoryRepository
```

注意：上图中的接口名称是 `ICategoryRepository`。它与
`IAccountRepository` 分开，因为账户和分类未来会有不同查询能力，不能为了少写
一个文件而制造庞大的通用 Repository。

## 2. 文件变化

新增：

```text
src/features/accounting/domain/ICategoryRepository.h
src/features/accounting/application/CreateCategory.h
src/features/accounting/application/CreateCategory.cpp
tests/unit/accounting/support/InMemoryCategoryRepository.h
tests/unit/accounting/CategoryRepositoryTest.cpp
tests/unit/accounting/CreateCategoryTest.cpp
```

修改：

```text
src/features/accounting/domain/CMakeLists.txt
src/features/accounting/application/CMakeLists.txt
tests/unit/accounting/CMakeLists.txt
```

## 3. 定义 ICategoryRepository

创建 `src/features/accounting/domain/ICategoryRepository.h`：

```cpp
#pragma once

#include "Category.h"

#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::accounting {

class ICategoryRepository
{
public:
    virtual ~ICategoryRepository() = default;

    [[nodiscard]] virtual bool save(const Category &category) = 0;
    [[nodiscard]] virtual std::optional<Category> findById(
        const QUuid &id) const = 0;
    [[nodiscard]] virtual std::vector<Category> findAll() const = 0;
};

} // namespace stukit::accounting
```

接口只描述业务需要的能力，不包含：

- `QSqlDatabase`。
- 表名或 SQL 字段名。
- 页面列表模型。
- 错误提示文字。

将它加入 `src/features/accounting/domain/CMakeLists.txt`：

```cmake
    IAccountRepository.h
    ICategoryRepository.h
```

## 4. 创建内存测试实现

创建 `tests/unit/accounting/support/InMemoryCategoryRepository.h`：

```cpp
#pragma once

#include "ICategoryRepository.h"

#include <algorithm>
#include <utility>

namespace stukit::accounting::testing {

class InMemoryCategoryRepository final : public ICategoryRepository
{
public:
    [[nodiscard]] bool save(const Category &category) override
    {
        if (!m_saveShouldSucceed) {
            return false;
        }

        const auto existing = std::find_if(
            m_categories.begin(),
            m_categories.end(),
            [&category](const Category &stored) {
                return stored.id() == category.id();
            });

        if (existing == m_categories.end()) {
            m_categories.push_back(category);
        } else {
            *existing = category;
        }

        return true;
    }

    [[nodiscard]] std::optional<Category> findById(
        const QUuid &id) const override
    {
        const auto category = std::find_if(
            m_categories.cbegin(),
            m_categories.cend(),
            [&id](const Category &stored) {
                return stored.id() == id;
            });

        if (category == m_categories.cend()) {
            return std::nullopt;
        }

        return *category;
    }

    [[nodiscard]] std::vector<Category> findAll() const override
    {
        return m_categories;
    }

    void setSaveShouldSucceed(bool shouldSucceed) noexcept
    {
        m_saveShouldSucceed = shouldSucceed;
    }

private:
    std::vector<Category> m_categories;
    bool m_saveShouldSucceed{true};
};

} // namespace stukit::accounting::testing
```

这个实现只用于测试。它让 Application 测试不依赖文件、SQLite 驱动和数据库迁移，
还可以通过 `setSaveShouldSucceed(false)` 模拟存储失败。

## 5. 测试 Repository 契约

创建 `tests/unit/accounting/CategoryRepositoryTest.cpp`：

```cpp
#include "support/InMemoryCategoryRepository.h"

#include <QTest>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

namespace {

Category makeCategory(
    const QUuid &id,
    const QString &name,
    TransactionType type = TransactionType::Expense)
{
    const auto category = Category::create(CategoryData{
        .id = id,
        .name = name,
        .type = type,
        .archived = false,
    });

    Q_ASSERT(category.has_value());
    return *category;
}

} // namespace

class CategoryRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void savesAndFindsCategory();
    void returnsNulloptForMissingCategory();
    void updatesCategoryWithSameId();
    void returnsAllCategories();
};

void CategoryRepositoryTest::savesAndFindsCategory()
{
    InMemoryCategoryRepository repository;
    const auto id = QUuid::createUuid();

    QVERIFY(repository.save(makeCategory(
        id, QStringLiteral("餐饮"), TransactionType::Expense)));

    const auto stored = repository.findById(id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->id(), id);
    QCOMPARE(stored->name(), QStringLiteral("餐饮"));
    QVERIFY(stored->type() == TransactionType::Expense);
}

void CategoryRepositoryTest::returnsNulloptForMissingCategory()
{
    const InMemoryCategoryRepository repository;

    QVERIFY(!repository.findById(QUuid::createUuid()).has_value());
}

void CategoryRepositoryTest::updatesCategoryWithSameId()
{
    InMemoryCategoryRepository repository;
    const auto id = QUuid::createUuid();

    QVERIFY(repository.save(makeCategory(id, QStringLiteral("餐饮"))));
    QVERIFY(repository.save(makeCategory(id, QStringLiteral("日常消费"))));

    const auto categories = repository.findAll();
    QCOMPARE(categories.size(), std::size_t{1});
    QCOMPARE(categories.front().name(), QStringLiteral("日常消费"));
}

void CategoryRepositoryTest::returnsAllCategories()
{
    InMemoryCategoryRepository repository;

    QVERIFY(repository.save(makeCategory(
        QUuid::createUuid(), QStringLiteral("餐饮"))));
    QVERIFY(repository.save(makeCategory(
        QUuid::createUuid(),
        QStringLiteral("工资"),
        TransactionType::Income)));

    QCOMPARE(repository.findAll().size(), std::size_t{2});
}

QTEST_APPLESS_MAIN(CategoryRepositoryTest)

#include "CategoryRepositoryTest.moc"
```

这组测试检查 Repository 的基本契约，而不是 SQLite。以后内存实现和 SQLite 实现
都应该表现出相同的新增、查询和同 ID 更新语义。

## 6. 定义 CreateCategory 用例

创建 `src/features/accounting/application/CreateCategory.h`：

```cpp
#pragma once

#include "ICategoryRepository.h"
#include "TransactionType.h"

#include <QString>

#include <variant>

namespace stukit::accounting {

struct CreateCategoryRequest
{
    QString name;
    TransactionType type{TransactionType::Expense};
};

enum class CreateCategoryError
{
    InvalidInput,
    StorageFailure
};

using CreateCategoryResult = std::variant<Category, CreateCategoryError>;

class CreateCategory final
{
public:
    explicit CreateCategory(ICategoryRepository &repository);

    [[nodiscard]] CreateCategoryResult execute(
        const CreateCategoryRequest &request);

private:
    ICategoryRepository &m_repository;
};

} // namespace stukit::accounting
```

这里保留收入和支出类型，因为“工资”和“餐饮”不能被当作相同类型的分类使用。

## 7. 实现 CreateCategory

创建 `src/features/accounting/application/CreateCategory.cpp`：

```cpp
#include "CreateCategory.h"

#include <QUuid>

namespace stukit::accounting {

CreateCategory::CreateCategory(ICategoryRepository &repository)
    : m_repository(repository)
{
}

CreateCategoryResult CreateCategory::execute(
    const CreateCategoryRequest &request)
{
    const auto category = Category::create(CategoryData{
        .id = QUuid::createUuid(),
        .name = request.name,
        .type = request.type,
        .archived = false,
    });

    if (!category.has_value()) {
        return CreateCategoryError::InvalidInput;
    }

    if (!m_repository.save(*category)) {
        return CreateCategoryError::StorageFailure;
    }

    return *category;
}

} // namespace stukit::accounting
```

这个流程与 CreateAccount 相似：

```text
生成 UUID -> 创建有效实体 -> 保存 -> 返回结果
```

相似是正常的，因为两个用例遵守同一种架构；不要为了消除几行重复而创建一个难以
理解的通用模板用例。

将两个文件加入 `src/features/accounting/application/CMakeLists.txt`：

```cmake
add_library(StuKitAccountingApplication STATIC
    CreateAccount.cpp
    CreateAccount.h
    CreateCategory.cpp
    CreateCategory.h
)
```

## 8. 测试 CreateCategory

创建 `tests/unit/accounting/CreateCategoryTest.cpp`：

```cpp
#include "CreateCategory.h"
#include "support/InMemoryCategoryRepository.h"

#include <QTest>

#include <variant>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

class CreateCategoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsAndStoresValidCategory();
    void rejectsInvalidInputWithoutSaving();
    void reportsStorageFailure();
    void generatesDifferentIds();
};

void CreateCategoryTest::createsAndStoresValidCategory()
{
    InMemoryCategoryRepository repository;
    CreateCategory createCategory(repository);

    const auto result = createCategory.execute(CreateCategoryRequest{
        .name = QStringLiteral("  工资  "),
        .type = TransactionType::Income,
    });

    QVERIFY(std::holds_alternative<Category>(result));
    const auto &category = std::get<Category>(result);
    QCOMPARE(category.name(), QStringLiteral("工资"));
    QVERIFY(category.type() == TransactionType::Income);
    QVERIFY(!category.isArchived());

    const auto stored = repository.findById(category.id());
    QVERIFY(stored.has_value());
    QCOMPARE(stored->name(), QStringLiteral("工资"));
}

void CreateCategoryTest::rejectsInvalidInputWithoutSaving()
{
    InMemoryCategoryRepository repository;
    CreateCategory createCategory(repository);

    const auto result = createCategory.execute(CreateCategoryRequest{
        .name = QStringLiteral("   "),
    });

    QVERIFY(std::holds_alternative<CreateCategoryError>(result));
    QVERIFY(std::get<CreateCategoryError>(result)
            == CreateCategoryError::InvalidInput);
    QVERIFY(repository.findAll().empty());
}

void CreateCategoryTest::reportsStorageFailure()
{
    InMemoryCategoryRepository repository;
    repository.setSaveShouldSucceed(false);
    CreateCategory createCategory(repository);

    const auto result = createCategory.execute(CreateCategoryRequest{
        .name = QStringLiteral("餐饮"),
    });

    QVERIFY(std::holds_alternative<CreateCategoryError>(result));
    QVERIFY(std::get<CreateCategoryError>(result)
            == CreateCategoryError::StorageFailure);
    QVERIFY(repository.findAll().empty());
}

void CreateCategoryTest::generatesDifferentIds()
{
    InMemoryCategoryRepository repository;
    CreateCategory createCategory(repository);

    const auto first = createCategory.execute(CreateCategoryRequest{
        .name = QStringLiteral("餐饮"),
    });
    const auto second = createCategory.execute(CreateCategoryRequest{
        .name = QStringLiteral("交通"),
    });

    QVERIFY(std::holds_alternative<Category>(first));
    QVERIFY(std::holds_alternative<Category>(second));
    QVERIFY(std::get<Category>(first).id()
            != std::get<Category>(second).id());
}

QTEST_APPLESS_MAIN(CreateCategoryTest)

#include "CreateCategoryTest.moc"
```

## 9. 注册两组测试

在 `tests/unit/accounting/CMakeLists.txt` 末尾加入：

```cmake
add_executable(StuKitCategoryRepositoryTest
    CategoryRepositoryTest.cpp
    support/InMemoryCategoryRepository.h
)

target_link_libraries(StuKitCategoryRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingDomain
)

add_test(
    NAME accounting.category_repository
    COMMAND StuKitCategoryRepositoryTest
)

add_executable(StuKitCreateCategoryTest
    CreateCategoryTest.cpp
    support/InMemoryCategoryRepository.h
)

target_link_libraries(StuKitCreateCategoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingApplication
)

add_test(
    NAME accounting.create_category
    COMMAND StuKitCreateCategoryTest
)
```

每个包含 `QTEST_APPLESS_MAIN` 的测试文件都必须拥有独立可执行目标，否则链接时会
出现多个 `main` 函数。

## 10. 构建和测试

在 Qt Creator 中：

1. 执行“构建 > 运行 CMake”。
2. 重新构建项目。
3. 打开“测试结果”。
4. 运行全部测试。
5. 再运行 StuKit 应用，确认主窗口可以正常启动。

新增测试：

```text
accounting.category_repository
accounting.create_category
```

预期结果：

```text
10/10 tests passed
```

## 11. 本课暂不处理分类重名

当前用例允许创建两个同名分类。要可靠地区分“没有找到同名分类”和“数据库查询
失败”，需要先设计更清晰的 Repository 查询错误类型，而不是简单增加一个返回
`bool` 的函数。

因此本课只处理单个 Category 的有效性和保存失败。分类重名规则将在完善错误模型
时单独实现，不在这里用含义模糊的返回值草率处理。

## 12. 常见错误

### `ICategoryRepository.h` 找不到

确认文件已经加入 Domain 的 CMake 目标，并且 Application 继续公开链接
`StuKitAccountingDomain`。

### `std::variant` 未定义

`CreateCategory.h` 必须直接包含 `<variant>`。

### `TransactionType` 未定义

`CreateCategory.h` 应包含 `TransactionType.h`；测试文件通过 CreateCategory 头文件
获得这个类型，不要在每个位置重新定义枚举。

### 链接错误：main 已经定义

不要把 `CategoryRepositoryTest.cpp` 和 `CreateCategoryTest.cpp` 放入同一个
`add_executable()`。每个 Qt Test 类都需要独立目标。

### 测试找不到新目标

新增或修改 CMakeLists 后先执行“运行 CMake”，再构建和运行测试。

### Qt Creator 把文件加入 QML 模块

从根 `qt_add_qml_module()` 删除这些 C++ 文件。Domain、Application 和测试文件
只能加入各自的普通 CMake 目标。

## 13. 验收清单

- [ ] `ICategoryRepository` 位于 Domain，且没有 Qt SQL 依赖。
- [ ] 内存 Repository 能新增、按 ID 查询、更新和返回全部分类。
- [ ] 内存 Repository 可以模拟保存失败。
- [ ] `CreateCategory` 位于 Application。
- [ ] CreateCategory 生成 UUID 并调用 `Category::create()`。
- [ ] 无效名称不会调用保存。
- [ ] 保存失败返回 `StorageFailure`。
- [ ] 收入/支出类型能正确保留。
- [ ] 两个测试文件拥有独立 CMake 目标。
- [ ] 10 组测试全部通过。
- [ ] StuKit 应用仍能正常构建和运行。
- [ ] `git diff --check` 没有输出。

## 14. 完成后反馈

完成后先不要提交，把以下内容发给我：

```text
CMake 配置：成功 / 失败
CategoryRepositoryTest：成功 / 失败
CreateCategoryTest：成功 / 失败
10 组测试结果：
StuKit 应用：正常 / 异常
完整错误或警告：
git diff --stat 输出：
```

我会审查代码、解释问题、运行最终验证并帮你提交。第十四课将新增 categories 表和
SQLiteCategoryRepository；按照你的安排，这部分数据库实现由我负责。
