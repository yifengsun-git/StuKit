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
    QVERIFY(categories.has_value());
    QCOMPARE(categories->size(), std::size_t{1});
    QCOMPARE(categories->front().name(), QStringLiteral("日常消费"));
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

    const auto categories = repository.findAll();
    QVERIFY(categories.has_value());
    QCOMPARE(categories->size(), std::size_t{2});
}

QTEST_APPLESS_MAIN(CategoryRepositoryTest)

#include "CategoryRepositoryTest.moc"
