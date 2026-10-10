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
