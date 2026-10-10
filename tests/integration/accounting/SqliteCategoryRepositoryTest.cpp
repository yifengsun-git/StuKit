#include "CreateCategory.h"
#include "database/SqliteDatabase.h"
#include "repositories/SqliteCategoryRepository.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

#include <memory>
#include <variant>

using namespace stukit::accounting;

namespace {

QString uniqueConnectionName()
{
    return QStringLiteral("stukit-category-repository-test-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

Category makeCategory(const QUuid &id, const QString &name, TransactionType type,
                      bool archived = false)
{
    const auto category = Category::create(CategoryData{
        .id = id,
        .name = name,
        .type = type,
        .archived = archived,
    });

    Q_ASSERT(category.has_value());
    return *category;
}

} // namespace

class SqliteCategoryRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void savesAndFindsExpenseCategory();
    void savesAndFindsIncomeCategory();
    void returnsNulloptForMissingCategory();
    void updatesCategoryWithSameId();
    void returnsAllCategoriesInStableOrder();
    void worksWithCreateCategoryUseCase();

private:
    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<SqliteDatabase> m_database;
    std::unique_ptr<SqliteCategoryRepository> m_repository;
};

void SqliteCategoryRepositoryTest::init()
{
    m_directory = std::make_unique<QTemporaryDir>(QDir::current().filePath(
        QStringLiteral("stukit-category-repository-test-XXXXXX")));
    QVERIFY(m_directory->isValid());

    m_database = std::make_unique<SqliteDatabase>(uniqueConnectionName());
    QVERIFY2(m_database->open(m_directory->filePath(QStringLiteral("stukit.sqlite"))),
             qPrintable(m_database->lastError()));
    QVERIFY2(m_database->migrate(), qPrintable(m_database->lastError()));

    m_repository = std::make_unique<SqliteCategoryRepository>(
        m_database->connection());
}

void SqliteCategoryRepositoryTest::cleanup()
{
    m_repository.reset();
    m_database.reset();
    m_directory.reset();
}

void SqliteCategoryRepositoryTest::savesAndFindsExpenseCategory()
{
    const auto id = QUuid::createUuid();
    QVERIFY(m_repository->save(makeCategory(
        id, QStringLiteral("餐饮"), TransactionType::Expense, true)));

    const auto stored = m_repository->findById(id);
    QVERIFY2(stored.has_value(), qPrintable(m_repository->lastError()));
    QCOMPARE(stored->id(), id);
    QCOMPARE(stored->name(), QStringLiteral("餐饮"));
    QVERIFY(stored->type() == TransactionType::Expense);
    QVERIFY(stored->isArchived());
}

void SqliteCategoryRepositoryTest::savesAndFindsIncomeCategory()
{
    const auto id = QUuid::createUuid();
    QVERIFY(m_repository->save(
        makeCategory(id, QStringLiteral("工资"), TransactionType::Income)));

    const auto stored = m_repository->findById(id);
    QVERIFY2(stored.has_value(), qPrintable(m_repository->lastError()));
    QVERIFY(stored->type() == TransactionType::Income);
}

void SqliteCategoryRepositoryTest::returnsNulloptForMissingCategory()
{
    const auto stored = m_repository->findById(QUuid::createUuid());

    QVERIFY(!stored.has_value());
    QVERIFY(m_repository->lastError().isEmpty());
}

void SqliteCategoryRepositoryTest::updatesCategoryWithSameId()
{
    const auto id = QUuid::createUuid();
    QVERIFY(m_repository->save(
        makeCategory(id, QStringLiteral("餐饮"), TransactionType::Expense)));
    QVERIFY(m_repository->save(
        makeCategory(id, QStringLiteral("奖金"), TransactionType::Income)));

    const auto categories = m_repository->findAll();
    QVERIFY2(m_repository->lastError().isEmpty(),
             qPrintable(m_repository->lastError()));
    QVERIFY(categories.has_value());
    QCOMPARE(categories->size(), std::size_t{1});
    QCOMPARE(categories->front().name(), QStringLiteral("奖金"));
    QVERIFY(categories->front().type() == TransactionType::Income);
}

void SqliteCategoryRepositoryTest::returnsAllCategoriesInStableOrder()
{
    QVERIFY(m_repository->save(makeCategory(
        QUuid::createUuid(), QStringLiteral("餐饮"), TransactionType::Expense)));
    QVERIFY(m_repository->save(makeCategory(
        QUuid::createUuid(), QStringLiteral("工资"), TransactionType::Income)));

    const auto categories = m_repository->findAll();

    QVERIFY2(m_repository->lastError().isEmpty(),
             qPrintable(m_repository->lastError()));
    QVERIFY(categories.has_value());
    QCOMPARE(categories->size(), std::size_t{2});
    QVERIFY(categories->front().type() == TransactionType::Income);
    QVERIFY(categories->back().type() == TransactionType::Expense);
}

void SqliteCategoryRepositoryTest::worksWithCreateCategoryUseCase()
{
    CreateCategory createCategory(*m_repository);

    const auto result = createCategory.execute(CreateCategoryRequest{
        .name = QStringLiteral("交通"),
        .type = TransactionType::Expense,
    });

    QVERIFY(std::holds_alternative<Category>(result));
    const auto &created = std::get<Category>(result);

    const auto stored = m_repository->findById(created.id());
    QVERIFY2(stored.has_value(), qPrintable(m_repository->lastError()));
    QCOMPARE(stored->name(), QStringLiteral("交通"));
    QVERIFY(stored->type() == TransactionType::Expense);
}

QTEST_GUILESS_MAIN(SqliteCategoryRepositoryTest)

#include "SqliteCategoryRepositoryTest.moc"
