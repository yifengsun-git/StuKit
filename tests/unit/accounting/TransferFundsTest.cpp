#include "TransferFunds.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryCategoryRepository.h"
#include "support/InMemoryTransactionRepository.h"

#include <QTest>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

namespace {

Account makeAccount(const QString &name, bool archived = false)
{
    const auto account = Account::create(AccountData{
        .id = QUuid::createUuid(),
        .name = name,
        .openingBalance = Money::fromCents(0),
        .archived = archived,
    });
    Q_ASSERT(account.has_value());
    return *account;
}

Category makeCategory(const QString &name, TransactionType type)
{
    const auto category = Category::create(CategoryData{
        .id = QUuid::createUuid(),
        .name = name,
        .type = type,
        .archived = false,
    });
    Q_ASSERT(category.has_value());
    return *category;
}

struct Fixture
{
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    InMemoryTransactionRepository transactions;
    TransferFunds transfer{transactions, accounts, categories};
    Account source{makeAccount(QStringLiteral("银行卡"))};
    Account destination{makeAccount(QStringLiteral("校园卡"))};
    Category outgoing{makeCategory(
        QStringLiteral("账户转出"), TransactionType::Expense)};
    Category incoming{makeCategory(
        QStringLiteral("账户转入"), TransactionType::Income)};
    Category fee{makeCategory(
        QStringLiteral("手续费"), TransactionType::Expense)};

    Fixture()
    {
        // The saves must run outside Q_ASSERT: NDEBUG compiles the assertion away, which
        // previously left the repositories empty in Release builds.
        const bool accountsSaved =
            accounts.save(source) && accounts.save(destination);
        const bool categoriesSaved = categories.save(outgoing)
            && categories.save(incoming) && categories.save(fee);
        Q_ASSERT(accountsSaved);
        Q_ASSERT(categoriesSaved);
        Q_UNUSED(accountsSaved);
        Q_UNUSED(categoriesSaved);
    }

    TransferFundsRequest request() const
    {
        return {
            .amount = Money::fromCents(10000),
            .fee = Money::fromCents(200),
            .sourceAccountId = source.id(),
            .destinationAccountId = destination.id(),
            .outgoingCategoryId = outgoing.id(),
            .incomingCategoryId = incoming.id(),
            .feeCategoryId = fee.id(),
            .occurredAt = QDateTime(
                QDate(2026, 11, 1), QTime(10, 30), QTimeZone::UTC),
            .note = QStringLiteral("充值"),
        };
    }
};

} // namespace

class TransferFundsTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsOutgoingIncomingAndFeeTransactions();
    void omitsFeeTransactionWhenFeeIsZero();
    void rejectsSameAccountAndMismatchedCategories();
    void reportsStorageFailureWithoutCreatingTransactions();
};

void TransferFundsTest::createsOutgoingIncomingAndFeeTransactions()
{
    Fixture fixture;
    const auto result = fixture.transfer.execute(fixture.request());
    QVERIFY(std::holds_alternative<std::vector<Transaction>>(result));

    const auto &created = std::get<std::vector<Transaction>>(result);
    QCOMPARE(created.size(), std::size_t{3});
    QVERIFY(created[0].type() == TransactionType::Expense);
    QCOMPARE(created[0].accountId(), fixture.source.id());
    QCOMPARE(created[0].amount(), Money::fromCents(10000));
    QVERIFY(created[0].note().contains(QStringLiteral("校园卡")));
    QVERIFY(created[1].type() == TransactionType::Income);
    QCOMPARE(created[1].accountId(), fixture.destination.id());
    QCOMPARE(created[1].amount(), Money::fromCents(10000));
    QVERIFY(created[2].type() == TransactionType::Expense);
    QCOMPARE(created[2].accountId(), fixture.source.id());
    QCOMPARE(created[2].amount(), Money::fromCents(200));
    QCOMPARE(fixture.transactions.findAll()->size(), std::size_t{3});
}

void TransferFundsTest::omitsFeeTransactionWhenFeeIsZero()
{
    Fixture fixture;
    auto request = fixture.request();
    request.fee = Money::fromCents(0);
    request.feeCategoryId = {};

    const auto result = fixture.transfer.execute(request);
    QVERIFY(std::holds_alternative<std::vector<Transaction>>(result));
    QCOMPARE(std::get<std::vector<Transaction>>(result).size(), std::size_t{2});
}

void TransferFundsTest::rejectsSameAccountAndMismatchedCategories()
{
    Fixture fixture;
    auto request = fixture.request();
    request.destinationAccountId = request.sourceAccountId;
    QCOMPARE(std::get<TransferFundsError>(fixture.transfer.execute(request)),
             TransferFundsError::SameAccount);

    request = fixture.request();
    request.incomingCategoryId = fixture.outgoing.id();
    QCOMPARE(std::get<TransferFundsError>(fixture.transfer.execute(request)),
             TransferFundsError::CategoryTypeMismatch);
    QVERIFY(fixture.transactions.findAll()->empty());
}

void TransferFundsTest::reportsStorageFailureWithoutCreatingTransactions()
{
    Fixture fixture;
    fixture.transactions.setSaveShouldSucceed(false);
    QCOMPARE(
        std::get<TransferFundsError>(fixture.transfer.execute(fixture.request())),
        TransferFundsError::StorageFailure);
    QVERIFY(fixture.transactions.findAll()->empty());
}

QTEST_APPLESS_MAIN(TransferFundsTest)

#include "TransferFundsTest.moc"
