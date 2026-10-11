#include "AccountBalanceListModel.h"
#include "AccountBalancesViewModel.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryTransactionRepository.h"

#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include <limits>

using namespace stukit::accounting;
using namespace stukit::accounting::presentation;
using namespace stukit::accounting::testing;

namespace {

Account makeAccount(qint64 openingBalanceCents)
{
    return *Account::create(AccountData{
        .id = QUuid::createUuid(),
        .name = QStringLiteral("银行卡"),
        .openingBalance = Money::fromCents(openingBalanceCents),
        .archived = false,
    });
}

} // namespace

class AccountBalancesPresentationTest final : public QObject
{
    Q_OBJECT

private slots:
    void exposesFormattedBalances();
    void clearsDataAndMapsErrors();
};

void AccountBalancesPresentationTest::exposesFormattedBalances()
{
    InMemoryAccountRepository accounts;
    InMemoryTransactionRepository transactions;
    QVERIFY(accounts.save(makeAccount(12345)));
    GetAccountBalances getBalances(accounts, transactions);
    AccountBalancesViewModel viewModel(getBalances);
    QSignalSpy balancesSpy(&viewModel, &AccountBalancesViewModel::balancesChanged);

    viewModel.load();

    QCOMPARE(balancesSpy.count(), 1);
    QCOMPARE(viewModel.activeTotalText(), QStringLiteral("123.45"));
    QCOMPARE(viewModel.accounts()->rowCount(), 1);
    const QModelIndex index = viewModel.accounts()->index(0, 0);
    QCOMPARE(viewModel.accounts()->data(
                 index, AccountBalanceListModel::OpeningBalanceTextRole),
             QVariant(QStringLiteral("123.45")));
    QCOMPARE(viewModel.accounts()->data(
                 index, AccountBalanceListModel::CurrentBalanceTextRole),
             QVariant(QStringLiteral("123.45")));
    QVERIFY(!viewModel.hasError());
}

void AccountBalancesPresentationTest::clearsDataAndMapsErrors()
{
    InMemoryAccountRepository accounts;
    InMemoryTransactionRepository transactions;
    const Account account = makeAccount(std::numeric_limits<qint64>::max());
    QVERIFY(accounts.save(account));
    GetAccountBalances getBalances(accounts, transactions);
    AccountBalancesViewModel viewModel(getBalances);
    viewModel.load();
    QCOMPARE(viewModel.accounts()->rowCount(), 1);

    const auto transaction = Transaction::create(TransactionData{
        .id = QUuid::createUuid(),
        .type = TransactionType::Income,
        .amount = Money::fromCents(1),
        .accountId = account.id(),
        .categoryId = QUuid::createUuid(),
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .note = {},
    });
    QVERIFY(transaction.has_value());
    QVERIFY(transactions.save(*transaction));
    viewModel.load();
    QCOMPARE(viewModel.errorMessage(),
             QStringLiteral("账户余额超出支持范围"));
    QCOMPARE(viewModel.accounts()->rowCount(), 0);
    QCOMPARE(viewModel.activeTotalText(), QStringLiteral("0.00"));

    transactions.setFindAllShouldSucceed(false);
    viewModel.load();
    QCOMPARE(viewModel.errorMessage(), QStringLiteral("无法读取账户余额"));
}

QTEST_APPLESS_MAIN(AccountBalancesPresentationTest)

#include "AccountBalancesPresentationTest.moc"
