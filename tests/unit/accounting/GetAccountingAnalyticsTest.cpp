#include "GetAccountingAnalytics.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryCategoryRepository.h"
#include "support/InMemoryTransactionRepository.h"

#include <QTest>

#include <limits>
#include <variant>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

namespace {

QTimeZone utcPlusEight()
{
    return QTimeZone::fromSecondsAheadOfUtc(8 * 60 * 60);
}

Account makeAccount(
    const QUuid &id,
    const QString &name,
    bool archived = false)
{
    const auto account = Account::create({
        .id = id,
        .name = name,
        .openingBalance = Money::fromCents(0),
        .archived = archived,
    });
    Q_ASSERT(account.has_value());
    return *account;
}

Category makeCategory(
    const QUuid &id,
    const QString &name,
    TransactionType type,
    bool archived = false)
{
    const auto category = Category::create({
        .id = id,
        .name = name,
        .type = type,
        .archived = archived,
    });
    Q_ASSERT(category.has_value());
    return *category;
}

Transaction makeTransaction(
    TransactionType type,
    qint64 amountCents,
    const QString &utcTime,
    const QUuid &accountId,
    const QUuid &categoryId)
{
    const auto transaction = Transaction::create({
        .id = QUuid::createUuid(),
        .type = type,
        .amount = Money::fromCents(amountCents),
        .accountId = accountId,
        .categoryId = categoryId,
        .occurredAt = QDateTime::fromString(utcTime, Qt::ISODateWithMs),
        .note = {},
    });
    Q_ASSERT(transaction.has_value());
    return *transaction;
}

AccountingAnalytics requireAnalytics(GetAccountingAnalyticsResult result)
{
    Q_ASSERT(std::holds_alternative<AccountingAnalytics>(result));
    return std::get<AccountingAnalytics>(std::move(result));
}

struct Fixture
{
    Fixture()
        : cashId(QUuid::createUuid())
        , cardId(QUuid::createUuid())
        , salaryId(QUuid::createUuid())
        , diningId(QUuid::createUuid())
        , transitId(QUuid::createUuid())
        , getAnalytics(transactions, accounts, categories)
    {
        const bool cashSaved = accounts.save(makeAccount(
            cashId, QStringLiteral("现金")));
        const bool cardSaved = accounts.save(makeAccount(
            cardId, QStringLiteral("旧银行卡"), true));
        const bool salarySaved = categories.save(makeCategory(
            salaryId, QStringLiteral("工资"), TransactionType::Income));
        const bool diningSaved = categories.save(makeCategory(
            diningId, QStringLiteral("餐饮"),
            TransactionType::Expense, true));
        const bool transitSaved = categories.save(makeCategory(
            transitId, QStringLiteral("交通"), TransactionType::Expense));
        Q_ASSERT(cashSaved && cardSaved && salarySaved
                 && diningSaved && transitSaved);
        Q_UNUSED(cashSaved);
        Q_UNUSED(cardSaved);
        Q_UNUSED(salarySaved);
        Q_UNUSED(diningSaved);
        Q_UNUSED(transitSaved);
    }

    InMemoryTransactionRepository transactions;
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    QUuid cashId;
    QUuid cardId;
    QUuid salaryId;
    QUuid diningId;
    QUuid transitId;
    GetAccountingAnalytics getAnalytics;
};

} // namespace

class GetAccountingAnalyticsTest final : public QObject
{
    Q_OBJECT

private slots:
    void returnsContinuousZeroMonthsForEmptyData();
    void aggregatesMonthlyCategoryAndAccountData();
    void appliesAccountAndCategoryFilters();
    void usesAccountingTimeZoneAcrossYearBoundary();
    void reportsInvalidInputAndStorageFailures();
    void reportsArithmeticOverflow();
};

void GetAccountingAnalyticsTest::returnsContinuousZeroMonthsForEmptyData()
{
    Fixture fixture;

    const auto analytics = requireAnalytics(fixture.getAnalytics.execute({
        .selectedMonth = QDate(2026, 10, 20),
        .monthCount = 6,
        .timeZone = utcPlusEight(),
    }));

    QCOMPARE(analytics.months.size(), std::size_t{6});
    QCOMPARE(analytics.months.front().month, QDate(2026, 5, 1));
    QCOMPARE(analytics.months.back().month, QDate(2026, 10, 1));
    for (const MonthlyAnalyticsPoint &point : analytics.months) {
        QCOMPARE(point.income, Money::fromCents(0));
        QCOMPARE(point.expense, Money::fromCents(0));
        QCOMPARE(point.balance, Money::fromCents(0));
    }
    QVERIFY(analytics.incomeCategories.empty());
    QVERIFY(analytics.expenseCategories.empty());
    QVERIFY(analytics.accountFlows.empty());
    QCOMPARE(fixture.transactions.findOccurredBetweenCallCount(), 1);
    QCOMPARE(fixture.accounts.findAllCallCount(), 1);
    QCOMPARE(fixture.categories.findAllCallCount(), 1);
}

void GetAccountingAnalyticsTest::aggregatesMonthlyCategoryAndAccountData()
{
    Fixture fixture;
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Expense, 100,
        QStringLiteral("2026-08-31T16:30:00.000Z"),
        fixture.cashId, fixture.diningId)));
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Income, 10000,
        QStringLiteral("2026-10-10T01:00:00.000Z"),
        fixture.cashId, fixture.salaryId)));
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Expense, 2500,
        QStringLiteral("2026-10-10T02:00:00.000Z"),
        fixture.cashId, fixture.diningId)));
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Expense, 750,
        QStringLiteral("2026-10-11T02:00:00.000Z"),
        fixture.cashId, fixture.diningId)));
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Expense, 100,
        QStringLiteral("2026-10-12T02:00:00.000Z"),
        fixture.cardId, fixture.transitId)));

    const auto analytics = requireAnalytics(fixture.getAnalytics.execute({
        .selectedMonth = QDate(2026, 10, 1),
        .monthCount = 2,
        .timeZone = utcPlusEight(),
    }));

    QCOMPARE(analytics.months.at(0).month, QDate(2026, 9, 1));
    QCOMPARE(analytics.months.at(0).expense, Money::fromCents(100));
    QCOMPARE(analytics.months.at(0).balance, Money::fromCents(-100));
    QCOMPARE(analytics.months.at(1).income, Money::fromCents(10000));
    QCOMPARE(analytics.months.at(1).expense, Money::fromCents(3350));
    QCOMPARE(analytics.months.at(1).balance, Money::fromCents(6650));

    QCOMPARE(analytics.incomeCategories.size(), std::size_t{1});
    QCOMPARE(analytics.incomeCategories.front().categoryName,
             QStringLiteral("工资"));
    QCOMPARE(analytics.incomeCategories.front().amount,
             Money::fromCents(10000));
    QCOMPARE(analytics.expenseCategories.size(), std::size_t{2});
    QCOMPARE(analytics.expenseCategories.front().categoryName,
             QStringLiteral("餐饮"));
    QCOMPARE(analytics.expenseCategories.front().amount,
             Money::fromCents(3250));
    QCOMPARE(analytics.expenseCategories.front().transactionCount, 2);
    QVERIFY(analytics.expenseCategories.front().archived);

    QCOMPARE(analytics.accountFlows.size(), std::size_t{2});
    QCOMPARE(analytics.accountFlows.front().accountName,
             QStringLiteral("现金"));
    QCOMPARE(analytics.accountFlows.front().income,
             Money::fromCents(10000));
    QCOMPARE(analytics.accountFlows.front().expense,
             Money::fromCents(3250));
    QCOMPARE(analytics.accountFlows.front().netFlow,
             Money::fromCents(6750));
    QCOMPARE(analytics.accountFlows.back().accountName,
             QStringLiteral("旧银行卡"));
    QVERIFY(analytics.accountFlows.back().archived);
}

void GetAccountingAnalyticsTest::appliesAccountAndCategoryFilters()
{
    Fixture fixture;
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Expense, 500,
        QStringLiteral("2026-10-10T02:00:00.000Z"),
        fixture.cashId, fixture.diningId)));
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Expense, 900,
        QStringLiteral("2026-10-11T02:00:00.000Z"),
        fixture.cardId, fixture.diningId)));
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Expense, 300,
        QStringLiteral("2026-10-12T02:00:00.000Z"),
        fixture.cashId, fixture.transitId)));

    const auto analytics = requireAnalytics(fixture.getAnalytics.execute({
        .selectedMonth = QDate(2026, 10, 1),
        .monthCount = 2,
        .timeZone = utcPlusEight(),
        .accountIds = {fixture.cashId},
        .categoryIds = {fixture.diningId},
    }));

    QCOMPARE(analytics.months.back().expense, Money::fromCents(500));
    QCOMPARE(analytics.expenseCategories.size(), std::size_t{1});
    QCOMPARE(analytics.expenseCategories.front().amount,
             Money::fromCents(500));
    QCOMPARE(analytics.accountFlows.size(), std::size_t{1});
    QCOMPARE(analytics.accountFlows.front().accountId, fixture.cashId);
}

void GetAccountingAnalyticsTest::usesAccountingTimeZoneAcrossYearBoundary()
{
    Fixture fixture;
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Expense, 888,
        QStringLiteral("2025-12-31T16:00:00.000Z"),
        fixture.cashId, fixture.diningId)));

    const auto analytics = requireAnalytics(fixture.getAnalytics.execute({
        .selectedMonth = QDate(2026, 1, 1),
        .monthCount = 2,
        .timeZone = utcPlusEight(),
    }));

    QCOMPARE(analytics.months.front().month, QDate(2025, 12, 1));
    QCOMPARE(analytics.months.front().expense, Money::fromCents(0));
    QCOMPARE(analytics.months.back().month, QDate(2026, 1, 1));
    QCOMPARE(analytics.months.back().expense, Money::fromCents(888));
}

void GetAccountingAnalyticsTest::reportsInvalidInputAndStorageFailures()
{
    Fixture fixture;
    const auto request = GetAccountingAnalyticsRequest{
        .selectedMonth = QDate(2026, 10, 1),
        .monthCount = 12,
        .timeZone = utcPlusEight(),
    };

    auto invalid = request;
    invalid.monthCount = 1;
    QCOMPARE(std::get<GetAccountingAnalyticsError>(
                 fixture.getAnalytics.execute(invalid)),
             GetAccountingAnalyticsError::InvalidInput);
    invalid = request;
    invalid.accountIds = {QUuid::createUuid()};
    QCOMPARE(std::get<GetAccountingAnalyticsError>(
                 fixture.getAnalytics.execute(invalid)),
             GetAccountingAnalyticsError::InvalidInput);

    fixture.transactions.setFindOccurredBetweenShouldSucceed(false);
    QCOMPARE(std::get<GetAccountingAnalyticsError>(
                 fixture.getAnalytics.execute(request)),
             GetAccountingAnalyticsError::StorageFailure);
    fixture.transactions.setFindOccurredBetweenShouldSucceed(true);

    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Expense, 100,
        QStringLiteral("2026-10-10T02:00:00.000Z"),
        QUuid::createUuid(), fixture.diningId)));
    QCOMPARE(std::get<GetAccountingAnalyticsError>(
                 fixture.getAnalytics.execute(request)),
             GetAccountingAnalyticsError::StorageFailure);
}

void GetAccountingAnalyticsTest::reportsArithmeticOverflow()
{
    Fixture fixture;
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Income,
        std::numeric_limits<qint64>::max(),
        QStringLiteral("2026-10-10T01:00:00.000Z"),
        fixture.cashId, fixture.salaryId)));
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Income, 1,
        QStringLiteral("2026-10-11T01:00:00.000Z"),
        fixture.cashId, fixture.salaryId)));

    QCOMPARE(std::get<GetAccountingAnalyticsError>(
                 fixture.getAnalytics.execute({
                     .selectedMonth = QDate(2026, 10, 1),
                     .monthCount = 2,
                     .timeZone = utcPlusEight(),
                 })),
             GetAccountingAnalyticsError::ArithmeticOverflow);
}

QTEST_APPLESS_MAIN(GetAccountingAnalyticsTest)

#include "GetAccountingAnalyticsTest.moc"
