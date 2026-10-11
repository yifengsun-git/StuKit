#include "GetMonthlyOverview.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryCategoryRepository.h"
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

Account makeAccount(const QUuid &id, bool archived = false)
{
    const auto account = Account::create(AccountData{
        .id = id,
        .name = QStringLiteral("银行卡"),
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
    const auto category = Category::create(CategoryData{
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
    const auto transaction = Transaction::create(TransactionData{
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

MonthlyOverview requireOverview(const GetMonthlyOverviewResult &result)
{
    Q_ASSERT(std::holds_alternative<MonthlyOverview>(result));
    return std::get<MonthlyOverview>(result);
}

struct Fixture
{
    Fixture()
        : accountId(QUuid::createUuid())
        , incomeCategoryId(QUuid::createUuid())
        , expenseCategoryId(QUuid::createUuid())
        , getMonthlyOverview(transactions, accounts, categories)
    {
        const bool accountSaved = accounts.save(makeAccount(accountId));
        const bool incomeSaved = categories.save(makeCategory(
            incomeCategoryId, QStringLiteral("工资"), TransactionType::Income));
        const bool expenseSaved = categories.save(makeCategory(
            expenseCategoryId, QStringLiteral("餐饮"), TransactionType::Expense));
        Q_ASSERT(accountSaved && incomeSaved && expenseSaved);
        Q_UNUSED(accountSaved);
        Q_UNUSED(incomeSaved);
        Q_UNUSED(expenseSaved);
    }

    Transaction transaction(
        TransactionType type, qint64 amountCents, const QString &utcTime) const
    {
        return makeTransaction(
            type,
            amountCents,
            utcTime,
            accountId,
            type == TransactionType::Income
                ? incomeCategoryId
                : expenseCategoryId);
    }

    InMemoryTransactionRepository transactions;
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    QUuid accountId;
    QUuid incomeCategoryId;
    QUuid expenseCategoryId;
    GetMonthlyOverview getMonthlyOverview;
};

} // namespace

class GetMonthlyOverviewTest final : public QObject
{
    Q_OBJECT

private slots:
    void returnsZeroTotalsForEmptyMonth();
    void calculatesTotalsAndAttachesNames();
    void usesCustomTimeZoneAtMonthBoundaries();
    void handlesDecemberAcrossYearBoundary();
    void loadsRelatedCollectionsOnlyOnce();
    void retainsNamesForArchivedRelations();
    void rejectsInvalidMonthOrTimeZone();
    void reportsRepositoryOrMissingRelationFailure();
};

void GetMonthlyOverviewTest::returnsZeroTotalsForEmptyMonth()
{
    Fixture fixture;
    const auto result = fixture.getMonthlyOverview.execute({
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

void GetMonthlyOverviewTest::calculatesTotalsAndAttachesNames()
{
    Fixture fixture;
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Income, 7500,
        QStringLiteral("2026-10-10T02:00:00.000Z"))));
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Expense, 8000,
        QStringLiteral("2026-10-12T02:00:00.000Z"))));

    const auto overview = requireOverview(fixture.getMonthlyOverview.execute({
        .year = 2026,
        .month = 10,
        .timeZone = customUtcPlusEight(),
    }));

    QCOMPARE(overview.transactions.size(), std::size_t{2});
    QCOMPARE(overview.totalIncome, Money::fromCents(7500));
    QCOMPARE(overview.totalExpense, Money::fromCents(8000));
    QCOMPARE(overview.balance, Money::fromCents(-500));
    QCOMPARE(overview.transactions.front().accountName, QStringLiteral("银行卡"));
    QCOMPARE(overview.transactions.front().categoryName, QStringLiteral("餐饮"));
    QCOMPARE(overview.transactions.back().categoryName, QStringLiteral("工资"));
}

void GetMonthlyOverviewTest::usesCustomTimeZoneAtMonthBoundaries()
{
    Fixture fixture;
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Expense, 100,
        QStringLiteral("2026-09-30T15:59:59.999Z"))));
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Expense, 200,
        QStringLiteral("2026-09-30T16:00:00.000Z"))));
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Expense, 300,
        QStringLiteral("2026-10-31T15:59:59.999Z"))));
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Expense, 400,
        QStringLiteral("2026-10-31T16:00:00.000Z"))));

    const auto overview = requireOverview(fixture.getMonthlyOverview.execute({
        .year = 2026,
        .month = 10,
        .timeZone = customUtcPlusEight(),
    }));

    QCOMPARE(overview.transactions.size(), std::size_t{2});
    QCOMPARE(overview.totalExpense, Money::fromCents(500));
    QCOMPARE(overview.transactions.front().transaction.amount(),
             Money::fromCents(300));
    QCOMPARE(overview.transactions.back().transaction.amount(),
             Money::fromCents(200));
}

void GetMonthlyOverviewTest::handlesDecemberAcrossYearBoundary()
{
    Fixture fixture;
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Income, 1000,
        QStringLiteral("2026-12-31T15:59:59.999Z"))));
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Income, 2000,
        QStringLiteral("2026-12-31T16:00:00.000Z"))));

    const auto overview = requireOverview(fixture.getMonthlyOverview.execute({
        .year = 2026,
        .month = 12,
        .timeZone = customUtcPlusEight(),
    }));

    QCOMPARE(overview.transactions.size(), std::size_t{1});
    QCOMPARE(overview.totalIncome, Money::fromCents(1000));
}

void GetMonthlyOverviewTest::loadsRelatedCollectionsOnlyOnce()
{
    Fixture fixture;
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Income, 1000,
        QStringLiteral("2026-10-10T02:00:00.000Z"))));
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Expense, 500,
        QStringLiteral("2026-10-11T02:00:00.000Z"))));

    fixture.getMonthlyOverview.execute({
        .year = 2026,
        .month = 10,
        .timeZone = customUtcPlusEight(),
    });

    QCOMPARE(fixture.accounts.findAllCallCount(), 1);
    QCOMPARE(fixture.categories.findAllCallCount(), 1);
}

void GetMonthlyOverviewTest::retainsNamesForArchivedRelations()
{
    Fixture fixture;
    QVERIFY(fixture.accounts.save(makeAccount(fixture.accountId, true)));
    QVERIFY(fixture.categories.save(makeCategory(
        fixture.expenseCategoryId,
        QStringLiteral("餐饮"),
        TransactionType::Expense,
        true)));
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Expense, 500,
        QStringLiteral("2026-10-10T02:00:00.000Z"))));

    const auto overview = requireOverview(fixture.getMonthlyOverview.execute({
        .year = 2026,
        .month = 10,
        .timeZone = customUtcPlusEight(),
    }));

    QCOMPARE(overview.transactions.front().accountName, QStringLiteral("银行卡"));
    QCOMPARE(overview.transactions.front().categoryName, QStringLiteral("餐饮"));
}

void GetMonthlyOverviewTest::rejectsInvalidMonthOrTimeZone()
{
    Fixture fixture;
    const auto invalidMonth = fixture.getMonthlyOverview.execute({
        .year = 2026,
        .month = 13,
        .timeZone = customUtcPlusEight(),
    });
    const auto invalidTimeZone = fixture.getMonthlyOverview.execute({
        .year = 2026,
        .month = 10,
        .timeZone = QTimeZone{},
    });

    QCOMPARE(std::get<GetMonthlyOverviewError>(invalidMonth),
             GetMonthlyOverviewError::InvalidInput);
    QCOMPARE(std::get<GetMonthlyOverviewError>(invalidTimeZone),
             GetMonthlyOverviewError::InvalidInput);
}

void GetMonthlyOverviewTest::reportsRepositoryOrMissingRelationFailure()
{
    Fixture transactionFailure;
    transactionFailure.transactions.setFindOccurredBetweenShouldSucceed(false);
    QVERIFY(std::holds_alternative<GetMonthlyOverviewError>(
        transactionFailure.getMonthlyOverview.execute({
            .year = 2026,
            .month = 10,
            .timeZone = customUtcPlusEight(),
        })));

    Fixture accountFailure;
    accountFailure.accounts.setFindAllShouldSucceed(false);
    QVERIFY(std::holds_alternative<GetMonthlyOverviewError>(
        accountFailure.getMonthlyOverview.execute({
            .year = 2026,
            .month = 10,
            .timeZone = customUtcPlusEight(),
        })));

    Fixture categoryFailure;
    categoryFailure.categories.setFindAllShouldSucceed(false);
    QVERIFY(std::holds_alternative<GetMonthlyOverviewError>(
        categoryFailure.getMonthlyOverview.execute({
            .year = 2026,
            .month = 10,
            .timeZone = customUtcPlusEight(),
        })));

    Fixture missingRelation;
    QVERIFY(missingRelation.transactions.save(makeTransaction(
        TransactionType::Expense,
        500,
        QStringLiteral("2026-10-10T02:00:00.000Z"),
        QUuid::createUuid(),
        missingRelation.expenseCategoryId)));
    const auto missingResult = missingRelation.getMonthlyOverview.execute({
        .year = 2026,
        .month = 10,
        .timeZone = customUtcPlusEight(),
    });
    QCOMPARE(std::get<GetMonthlyOverviewError>(missingResult),
             GetMonthlyOverviewError::StorageFailure);
}

QTEST_APPLESS_MAIN(GetMonthlyOverviewTest)

#include "GetMonthlyOverviewTest.moc"
