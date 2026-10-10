#include "GetMonthlyOverview.h"
#include "support/InMemoryTransactionRepository.h"

#include <QTest>

#include <variant>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

namespace {

QTimeZone customUtcPlusEight()
{
    return QTimeZone::fromSecondsAheadOfUtc(8 * 60 * 60);
}

Transaction makeTransaction(TransactionType type, qint64 amountCents,
                            const QString &utcTime)
{
    const auto transaction = Transaction::create(TransactionData{
        .id = QUuid::createUuid(),
        .type = type,
        .amount = Money::fromCents(amountCents),
        .accountId = QUuid::createUuid(),
        .categoryId = QUuid::createUuid(),
        .occurredAt = QDateTime::fromString(utcTime, Qt::ISODateWithMs),
        .note = {},
    });
    Q_ASSERT(transaction.has_value());
    return *transaction;
}

MonthlyOverview requireOverview(const GetMonthlyOverviewResult &result)
{
    Q_ASSERT(std::holds_alternative<MonthlyOverview>(result));
    return std::get<MonthlyOverview>(result);
}

} // namespace

class GetMonthlyOverviewTest final : public QObject
{
    Q_OBJECT

private slots:
    void returnsZeroTotalsForEmptyMonth();
    void calculatesIncomeExpenseAndNegativeBalance();
    void usesCustomTimeZoneAtMonthBoundaries();
    void handlesDecemberAcrossYearBoundary();
    void rejectsInvalidMonthOrTimeZone();
    void reportsStorageFailure();
};

void GetMonthlyOverviewTest::returnsZeroTotalsForEmptyMonth()
{
    InMemoryTransactionRepository repository;
    GetMonthlyOverview getMonthlyOverview(repository);

    const auto result = getMonthlyOverview.execute({
        .year = 2026,
        .month = 10,
        .timeZone = customUtcPlusEight(),
    });

    QVERIFY(std::holds_alternative<MonthlyOverview>(result));
    const auto overview = requireOverview(result);
    QVERIFY(overview.transactions.empty());
    QCOMPARE(overview.totalIncome, Money::fromCents(0));
    QCOMPARE(overview.totalExpense, Money::fromCents(0));
    QCOMPARE(overview.balance, Money::fromCents(0));
}

void GetMonthlyOverviewTest::calculatesIncomeExpenseAndNegativeBalance()
{
    InMemoryTransactionRepository repository;
    QVERIFY(repository.save(makeTransaction(
        TransactionType::Income, 5000,
        QStringLiteral("2026-10-10T02:00:00.000Z"))));
    QVERIFY(repository.save(makeTransaction(
        TransactionType::Income, 2500,
        QStringLiteral("2026-10-11T02:00:00.000Z"))));
    QVERIFY(repository.save(makeTransaction(
        TransactionType::Expense, 8000,
        QStringLiteral("2026-10-12T02:00:00.000Z"))));
    GetMonthlyOverview getMonthlyOverview(repository);

    const auto overview = requireOverview(getMonthlyOverview.execute({
        .year = 2026,
        .month = 10,
        .timeZone = customUtcPlusEight(),
    }));

    QCOMPARE(overview.transactions.size(), std::size_t{3});
    QCOMPARE(overview.totalIncome, Money::fromCents(7500));
    QCOMPARE(overview.totalExpense, Money::fromCents(8000));
    QCOMPARE(overview.balance, Money::fromCents(-500));
}

void GetMonthlyOverviewTest::usesCustomTimeZoneAtMonthBoundaries()
{
    InMemoryTransactionRepository repository;
    QVERIFY(repository.save(makeTransaction(
        TransactionType::Expense, 100,
        QStringLiteral("2026-09-30T15:59:59.999Z"))));
    QVERIFY(repository.save(makeTransaction(
        TransactionType::Expense, 200,
        QStringLiteral("2026-09-30T16:00:00.000Z"))));
    QVERIFY(repository.save(makeTransaction(
        TransactionType::Expense, 300,
        QStringLiteral("2026-10-31T15:59:59.999Z"))));
    QVERIFY(repository.save(makeTransaction(
        TransactionType::Expense, 400,
        QStringLiteral("2026-10-31T16:00:00.000Z"))));
    GetMonthlyOverview getMonthlyOverview(repository);

    const auto overview = requireOverview(getMonthlyOverview.execute({
        .year = 2026,
        .month = 10,
        .timeZone = customUtcPlusEight(),
    }));

    QCOMPARE(overview.transactions.size(), std::size_t{2});
    QCOMPARE(overview.totalExpense, Money::fromCents(500));
    QCOMPARE(overview.transactions.front().amount(), Money::fromCents(300));
    QCOMPARE(overview.transactions.back().amount(), Money::fromCents(200));
}

void GetMonthlyOverviewTest::handlesDecemberAcrossYearBoundary()
{
    InMemoryTransactionRepository repository;
    QVERIFY(repository.save(makeTransaction(
        TransactionType::Income, 1000,
        QStringLiteral("2026-12-31T15:59:59.999Z"))));
    QVERIFY(repository.save(makeTransaction(
        TransactionType::Income, 2000,
        QStringLiteral("2026-12-31T16:00:00.000Z"))));
    GetMonthlyOverview getMonthlyOverview(repository);

    const auto overview = requireOverview(getMonthlyOverview.execute({
        .year = 2026,
        .month = 12,
        .timeZone = customUtcPlusEight(),
    }));

    QCOMPARE(overview.transactions.size(), std::size_t{1});
    QCOMPARE(overview.totalIncome, Money::fromCents(1000));
}

void GetMonthlyOverviewTest::rejectsInvalidMonthOrTimeZone()
{
    InMemoryTransactionRepository repository;
    GetMonthlyOverview getMonthlyOverview(repository);

    const auto invalidMonth = getMonthlyOverview.execute({
        .year = 2026,
        .month = 13,
        .timeZone = customUtcPlusEight(),
    });
    const auto invalidTimeZone = getMonthlyOverview.execute({
        .year = 2026,
        .month = 10,
        .timeZone = QTimeZone{},
    });

    QVERIFY(std::get<GetMonthlyOverviewError>(invalidMonth)
            == GetMonthlyOverviewError::InvalidInput);
    QVERIFY(std::get<GetMonthlyOverviewError>(invalidTimeZone)
            == GetMonthlyOverviewError::InvalidInput);
}

void GetMonthlyOverviewTest::reportsStorageFailure()
{
    InMemoryTransactionRepository repository;
    repository.setFindOccurredBetweenShouldSucceed(false);
    GetMonthlyOverview getMonthlyOverview(repository);

    const auto result = getMonthlyOverview.execute({
        .year = 2026,
        .month = 10,
        .timeZone = customUtcPlusEight(),
    });

    QVERIFY(std::get<GetMonthlyOverviewError>(result)
            == GetMonthlyOverviewError::StorageFailure);
}

QTEST_APPLESS_MAIN(GetMonthlyOverviewTest)

#include "GetMonthlyOverviewTest.moc"
