#include "GetAccountBalances.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryTransactionRepository.h"

#include <QDateTime>
#include <QTest>
#include <QUuid>

#include <limits>
#include <variant>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

namespace {

Account makeAccount(
    const QUuid &id, const QString &name, qint64 openingBalanceCents,
    bool archived = false)
{
    return *Account::create(AccountData{
        .id = id,
        .name = name,
        .openingBalance = Money::fromCents(openingBalanceCents),
        .archived = archived,
    });
}

Transaction makeTransaction(
    TransactionType type, qint64 amountCents, const QUuid &accountId)
{
    return *Transaction::create(TransactionData{
        .id = QUuid::createUuid(),
        .type = type,
        .amount = Money::fromCents(amountCents),
        .accountId = accountId,
        .categoryId = QUuid::createUuid(),
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .note = {},
    });
}

AccountBalances requireBalances(GetAccountBalancesResult result)
{
    Q_ASSERT(std::holds_alternative<AccountBalances>(result));
    return std::get<AccountBalances>(std::move(result));
}

} // namespace

class GetAccountBalancesTest final : public QObject
{
    Q_OBJECT

private slots:
    void usesOpeningBalancesWhenThereAreNoTransactions();
    void calculatesEachAccountAndActiveTotal();
    void excludesArchivedAccountsOnlyFromTotal();
    void reportsRepositoryAndMissingAccountFailures();
    void reportsBalanceAndTotalOverflow();
};

void GetAccountBalancesTest::usesOpeningBalancesWhenThereAreNoTransactions()
{
    InMemoryAccountRepository accounts;
    InMemoryTransactionRepository transactions;
    QVERIFY(accounts.save(makeAccount(
        QUuid::createUuid(), QStringLiteral("现金"), -1234)));
    GetAccountBalances getBalances(accounts, transactions);

    const auto balances = requireBalances(getBalances.execute());

    QCOMPARE(balances.accounts.size(), std::size_t{1});
    QCOMPARE(balances.accounts.front().currentBalance,
             Money::fromCents(-1234));
    QCOMPARE(balances.activeTotal, Money::fromCents(-1234));
}

void GetAccountBalancesTest::calculatesEachAccountAndActiveTotal()
{
    InMemoryAccountRepository accounts;
    InMemoryTransactionRepository transactions;
    const QUuid cashId = QUuid::createUuid();
    const QUuid cardId = QUuid::createUuid();
    QVERIFY(accounts.save(makeAccount(
        cashId, QStringLiteral("现金"), 10000)));
    QVERIFY(accounts.save(makeAccount(
        cardId, QStringLiteral("银行卡"), -5000)));
    QVERIFY(transactions.save(makeTransaction(
        TransactionType::Income, 3000, cashId)));
    QVERIFY(transactions.save(makeTransaction(
        TransactionType::Expense, 1250, cashId)));
    QVERIFY(transactions.save(makeTransaction(
        TransactionType::Income, 10000, cardId)));
    GetAccountBalances getBalances(accounts, transactions);

    const auto balances = requireBalances(getBalances.execute());

    QCOMPARE(balances.accounts.at(0).currentBalance,
             Money::fromCents(11750));
    QCOMPARE(balances.accounts.at(1).currentBalance,
             Money::fromCents(5000));
    QCOMPARE(balances.activeTotal, Money::fromCents(16750));
}

void GetAccountBalancesTest::excludesArchivedAccountsOnlyFromTotal()
{
    InMemoryAccountRepository accounts;
    InMemoryTransactionRepository transactions;
    const QUuid archivedId = QUuid::createUuid();
    QVERIFY(accounts.save(makeAccount(
        archivedId, QStringLiteral("旧卡"), 2000, true)));
    QVERIFY(transactions.save(makeTransaction(
        TransactionType::Income, 500, archivedId)));
    GetAccountBalances getBalances(accounts, transactions);

    const auto balances = requireBalances(getBalances.execute());

    QCOMPARE(balances.accounts.front().currentBalance,
             Money::fromCents(2500));
    QCOMPARE(balances.activeTotal, Money::fromCents(0));
}

void GetAccountBalancesTest::reportsRepositoryAndMissingAccountFailures()
{
    InMemoryAccountRepository accounts;
    InMemoryTransactionRepository transactions;
    GetAccountBalances getBalances(accounts, transactions);

    accounts.setFindAllShouldSucceed(false);
    QCOMPARE(std::get<GetAccountBalancesError>(getBalances.execute()),
             GetAccountBalancesError::StorageFailure);

    accounts.setFindAllShouldSucceed(true);
    transactions.setFindAllShouldSucceed(false);
    QCOMPARE(std::get<GetAccountBalancesError>(getBalances.execute()),
             GetAccountBalancesError::StorageFailure);

    transactions.setFindAllShouldSucceed(true);
    QVERIFY(transactions.save(makeTransaction(
        TransactionType::Expense, 100, QUuid::createUuid())));
    QCOMPARE(std::get<GetAccountBalancesError>(getBalances.execute()),
             GetAccountBalancesError::StorageFailure);
}

void GetAccountBalancesTest::reportsBalanceAndTotalOverflow()
{
    InMemoryAccountRepository accounts;
    InMemoryTransactionRepository transactions;
    const QUuid accountId = QUuid::createUuid();
    QVERIFY(accounts.save(makeAccount(
        accountId,
        QStringLiteral("极限账户"),
        std::numeric_limits<qint64>::max())));
    QVERIFY(transactions.save(makeTransaction(
        TransactionType::Income, 1, accountId)));
    GetAccountBalances getBalances(accounts, transactions);
    QCOMPARE(std::get<GetAccountBalancesError>(getBalances.execute()),
             GetAccountBalancesError::ArithmeticOverflow);

    InMemoryAccountRepository totalAccounts;
    InMemoryTransactionRepository noTransactions;
    QVERIFY(totalAccounts.save(makeAccount(
        QUuid::createUuid(),
        QStringLiteral("账户一"),
        std::numeric_limits<qint64>::max())));
    QVERIFY(totalAccounts.save(makeAccount(
        QUuid::createUuid(), QStringLiteral("账户二"), 1)));
    GetAccountBalances getTotal(totalAccounts, noTransactions);
    QCOMPARE(std::get<GetAccountBalancesError>(getTotal.execute()),
             GetAccountBalancesError::ArithmeticOverflow);
}

QTEST_APPLESS_MAIN(GetAccountBalancesTest)

#include "GetAccountBalancesTest.moc"
