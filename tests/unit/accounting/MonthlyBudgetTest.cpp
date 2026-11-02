#include "ManageMonthlyBudgets.h"
#include "MonthlyBudget.h"
#include "MonthlyBudgetViewModel.h"
#include "support/InMemoryCategoryRepository.h"
#include "support/InMemoryMonthlyBudgetRepository.h"
#include "support/InMemoryTransactionRepository.h"

#include <QTest>

using namespace stukit::accounting;
using namespace stukit::accounting::presentation;
using namespace stukit::accounting::testing;

namespace {

Category makeCategory(
    const QUuid &id, const QString &name, TransactionType type,
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

Transaction makeExpense(const QUuid &categoryId, qint64 cents)
{
    const auto transaction = Transaction::create(TransactionData{
        .id = QUuid::createUuid(),
        .type = TransactionType::Expense,
        .amount = Money::fromCents(cents),
        .accountId = QUuid::createUuid(),
        .categoryId = categoryId,
        .occurredAt = QDateTime(QDate(2026, 11, 10), QTime(4, 0), Qt::UTC),
        .note = QStringLiteral("午餐"),
    });
    Q_ASSERT(transaction.has_value());
    return *transaction;
}

} // namespace

class MonthlyBudgetTest final : public QObject
{
    Q_OBJECT

private slots:
    void validatesBudgetValue();
    void calculatesProgressAndFiltersCategories();
    void viewModelSavesAndRemovesBudget();
};

void MonthlyBudgetTest::validatesBudgetValue()
{
    const QUuid categoryId = QUuid::createUuid();
    QVERIFY(MonthlyBudget::create(MonthlyBudgetData{
        .categoryId = categoryId,
        .year = 2026,
        .month = 11,
        .limit = Money::fromCents(50000),
    }).has_value());
    QVERIFY(!MonthlyBudget::create(MonthlyBudgetData{
        .categoryId = categoryId,
        .year = 2026,
        .month = 13,
        .limit = Money::fromCents(50000),
    }).has_value());
    QVERIFY(!MonthlyBudget::create(MonthlyBudgetData{
        .categoryId = categoryId,
        .year = 2026,
        .month = 11,
        .limit = Money::fromCents(0),
    }).has_value());
}

void MonthlyBudgetTest::calculatesProgressAndFiltersCategories()
{
    InMemoryMonthlyBudgetRepository budgets;
    InMemoryCategoryRepository categories;
    InMemoryTransactionRepository transactions;
    ManageMonthlyBudgets manage(budgets, categories, transactions);
    const QUuid foodId = QUuid::createUuid();
    const QUuid incomeId = QUuid::createUuid();
    QVERIFY(categories.save(makeCategory(
        foodId, QStringLiteral("餐饮"), TransactionType::Expense)));
    QVERIFY(categories.save(makeCategory(
        incomeId, QStringLiteral("工资"), TransactionType::Income)));
    QVERIFY(transactions.save(makeExpense(foodId, 12500)));
    QVERIFY(!manage.save(
        foodId, 2026, 11, Money::fromCents(50000)).has_value());

    const auto result = manage.overview(
        2026, 11, QTimeZone::fromSecondsAheadOfUtc(8 * 60 * 60));
    QVERIFY(std::holds_alternative<MonthlyBudgetOverview>(result));
    const auto &overview = std::get<MonthlyBudgetOverview>(result);
    QCOMPARE(overview.categories.size(), std::size_t{1});
    QCOMPARE(overview.categories.front().categoryName,
             QStringLiteral("餐饮"));
    QCOMPARE(overview.categories.front().limit->cents(), qint64{50000});
    QCOMPARE(overview.categories.front().spent.cents(), qint64{12500});

    const auto incomeBudgetError = manage.save(
        incomeId, 2026, 11, Money::fromCents(10000));
    QVERIFY(incomeBudgetError.has_value());
    QCOMPARE(*incomeBudgetError,
             ManageMonthlyBudgetError::CategoryTypeMismatch);
}

void MonthlyBudgetTest::viewModelSavesAndRemovesBudget()
{
    InMemoryMonthlyBudgetRepository budgets;
    InMemoryCategoryRepository categories;
    InMemoryTransactionRepository transactions;
    ManageMonthlyBudgets manage(budgets, categories, transactions);
    const QUuid foodId = QUuid::createUuid();
    QVERIFY(categories.save(makeCategory(
        foodId, QStringLiteral("餐饮"), TransactionType::Expense)));
    QVERIFY(transactions.save(makeExpense(foodId, 60000)));

    MonthlyBudgetViewModel viewModel(
        manage,
        QTimeZone::fromSecondsAheadOfUtc(8 * 60 * 60),
        QDate(2026, 11, 1));
    viewModel.load();
    QVERIFY(viewModel.hasCategories());
    QVERIFY(!viewModel.hasBudgets());
    QVERIFY(viewModel.saveBudget(
        foodId.toString(QUuid::WithoutBraces), QStringLiteral("500.00")));
    QVERIFY(viewModel.hasBudgets());
    const QVariantMap item = viewModel.categories().front().toMap();
    QCOMPARE(item.value(QStringLiteral("spentText")).toString(),
             QStringLiteral("600.00"));
    QCOMPARE(item.value(QStringLiteral("remainingText")).toString(),
             QStringLiteral("100.00"));
    QVERIFY(item.value(QStringLiteral("overBudget")).toBool());
    QCOMPARE(item.value(QStringLiteral("percentageText")).toString(),
             QStringLiteral("120%"));

    QVERIFY(viewModel.removeBudget(
        foodId.toString(QUuid::WithoutBraces)));
    QVERIFY(!viewModel.hasBudgets());
    QVERIFY(!viewModel.saveBudget(
        foodId.toString(QUuid::WithoutBraces), QStringLiteral("0")));
}

QTEST_APPLESS_MAIN(MonthlyBudgetTest)

#include "MonthlyBudgetTest.moc"
