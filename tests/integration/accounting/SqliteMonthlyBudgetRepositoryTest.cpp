#include "database/SqliteDatabase.h"
#include "repositories/SqliteCategoryRepository.h"
#include "repositories/SqliteMonthlyBudgetRepository.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

#include <memory>

using namespace stukit::accounting;

namespace {

QString connectionName()
{
    return QStringLiteral("stukit-budget-repository-test-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

Category makeExpenseCategory(const QUuid &id)
{
    const auto category = Category::create(CategoryData{
        .id = id,
        .name = QStringLiteral("餐饮"),
        .type = TransactionType::Expense,
        .archived = false,
    });
    Q_ASSERT(category.has_value());
    return *category;
}

MonthlyBudget makeBudget(const QUuid &categoryId, qint64 cents)
{
    const auto budget = MonthlyBudget::create(MonthlyBudgetData{
        .categoryId = categoryId,
        .year = 2026,
        .month = 11,
        .limit = Money::fromCents(cents),
    });
    Q_ASSERT(budget.has_value());
    return *budget;
}

} // namespace

class SqliteMonthlyBudgetRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void savesUpdatesRemovesAndCascadesBudget();

private:
    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<SqliteDatabase> m_database;
    std::unique_ptr<SqliteCategoryRepository> m_categories;
    std::unique_ptr<SqliteMonthlyBudgetRepository> m_budgets;
};

void SqliteMonthlyBudgetRepositoryTest::init()
{
    m_directory = std::make_unique<QTemporaryDir>(QDir::current().filePath(
        QStringLiteral("stukit-budget-repository-test-XXXXXX")));
    QVERIFY(m_directory->isValid());
    m_database = std::make_unique<SqliteDatabase>(connectionName());
    QVERIFY2(m_database->open(
                 m_directory->filePath(QStringLiteral("stukit.sqlite"))),
             qPrintable(m_database->lastError()));
    QVERIFY2(m_database->migrate(), qPrintable(m_database->lastError()));
    m_categories = std::make_unique<SqliteCategoryRepository>(
        m_database->connection());
    m_budgets = std::make_unique<SqliteMonthlyBudgetRepository>(
        m_database->connection());
}

void SqliteMonthlyBudgetRepositoryTest::cleanup()
{
    m_budgets.reset();
    m_categories.reset();
    m_database.reset();
    m_directory.reset();
}

void SqliteMonthlyBudgetRepositoryTest::savesUpdatesRemovesAndCascadesBudget()
{
    const QUuid categoryId = QUuid::createUuid();
    QVERIFY(m_categories->save(makeExpenseCategory(categoryId)));
    QVERIFY(m_budgets->save(makeBudget(categoryId, 50000)));
    auto stored = m_budgets->findForMonth(2026, 11);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->size(), std::size_t{1});
    QCOMPARE(stored->front().limit().cents(), qint64{50000});

    QVERIFY(m_budgets->save(makeBudget(categoryId, 80000)));
    stored = m_budgets->findForMonth(2026, 11);
    QCOMPARE(stored->size(), std::size_t{1});
    QCOMPARE(stored->front().limit().cents(), qint64{80000});
    QCOMPARE(m_budgets->remove(categoryId, 2026, 11),
             RemoveMonthlyBudgetResult::Removed);
    QCOMPARE(m_budgets->remove(categoryId, 2026, 11),
             RemoveMonthlyBudgetResult::NotFound);

    QVERIFY(m_budgets->save(makeBudget(categoryId, 50000)));
    QVERIFY(m_categories->remove(categoryId));
    stored = m_budgets->findForMonth(2026, 11);
    QVERIFY(stored->empty());
}

QTEST_GUILESS_MAIN(SqliteMonthlyBudgetRepositoryTest)

#include "SqliteMonthlyBudgetRepositoryTest.moc"
