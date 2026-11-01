#include "TransferFundsViewModel.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryCategoryRepository.h"
#include "support/InMemoryTransactionRepository.h"

#include <QSignalSpy>
#include <QTest>

using namespace stukit::accounting;
using namespace stukit::accounting::presentation;
using namespace stukit::accounting::testing;

namespace {

Account makeAccount(const QString &name)
{
    const auto account = Account::create(AccountData{
        .id = QUuid::createUuid(),
        .name = name,
        .openingBalance = Money::fromCents(0),
        .archived = false,
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

QString idText(const QUuid &id)
{
    return id.toString(QUuid::WithoutBraces);
}

} // namespace

class TransferFundsPresentationTest final : public QObject
{
    Q_OBJECT

private slots:
    void preparesOptionsAndCreatesTransferInCustomTimeZone();
    void requiresDistinctAccountsAndValidFee();
};

void TransferFundsPresentationTest::
    preparesOptionsAndCreatesTransferInCustomTimeZone()
{
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    InMemoryTransactionRepository transactions;
    const Account source = makeAccount(QStringLiteral("银行卡"));
    const Account destination = makeAccount(QStringLiteral("校园卡"));
    const Category expense = makeCategory(
        QStringLiteral("转出"), TransactionType::Expense);
    const Category income = makeCategory(
        QStringLiteral("转入"), TransactionType::Income);
    QVERIFY(accounts.save(source));
    QVERIFY(accounts.save(destination));
    QVERIFY(categories.save(expense));
    QVERIFY(categories.save(income));

    TransferFunds transfer(transactions, accounts, categories);
    GetAccountingSetup getSetup(accounts, categories);
    TransferFundsViewModel viewModel(
        transfer, getSetup,
        QTimeZone::fromSecondsAheadOfUtc(8 * 60 * 60));
    viewModel.prepare();

    QVERIFY(viewModel.isReady());
    QCOMPARE(viewModel.availableAccounts()->rowCount(), 2);
    QCOMPARE(viewModel.incomeCategories()->rowCount(), 1);
    QCOMPARE(viewModel.expenseCategories()->rowCount(), 1);
    QSignalSpy createdSpy(
        &viewModel, &TransferFundsViewModel::transferCreated);

    QVERIFY(viewModel.submit(
        QStringLiteral("100.00"), QStringLiteral("2.00"),
        idText(source.id()), idText(destination.id()),
        idText(expense.id()), idText(income.id()), idText(expense.id()),
        QStringLiteral("2026-11-01"), QStringLiteral("12:30"),
        QStringLiteral("充值")));
    QCOMPARE(createdSpy.count(), 1);
    const auto stored = transactions.findAll();
    QVERIFY(stored.has_value());
    QCOMPARE(stored->size(), std::size_t{3});
    QCOMPARE(stored->front().occurredAt().toUTC(),
             QDateTime(QDate(2026, 11, 1), QTime(4, 30), QTimeZone::UTC));
}

void TransferFundsPresentationTest::requiresDistinctAccountsAndValidFee()
{
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    InMemoryTransactionRepository transactions;
    const Account source = makeAccount(QStringLiteral("银行卡"));
    const Account destination = makeAccount(QStringLiteral("校园卡"));
    const Category expense = makeCategory(
        QStringLiteral("转出"), TransactionType::Expense);
    const Category income = makeCategory(
        QStringLiteral("转入"), TransactionType::Income);
    QVERIFY(accounts.save(source));
    QVERIFY(accounts.save(destination));
    QVERIFY(categories.save(expense));
    QVERIFY(categories.save(income));
    TransferFunds transfer(transactions, accounts, categories);
    GetAccountingSetup getSetup(accounts, categories);
    TransferFundsViewModel viewModel(
        transfer, getSetup, QTimeZone::UTC);
    viewModel.prepare();

    QVERIFY(!viewModel.submit(
        QStringLiteral("100"), QStringLiteral("-1"),
        idText(source.id()), idText(destination.id()),
        idText(expense.id()), idText(income.id()), idText(expense.id()),
        QStringLiteral("2026-11-01"), QStringLiteral("12:30"), {}));
    QCOMPARE(viewModel.errorMessage(),
             QStringLiteral("手续费必须是有效的非负金额"));

    QVERIFY(!viewModel.submit(
        QStringLiteral("100"), QStringLiteral("0"),
        idText(source.id()), idText(source.id()),
        idText(expense.id()), idText(income.id()), idText(expense.id()),
        QStringLiteral("2026-11-01"), QStringLiteral("12:30"), {}));
    QCOMPARE(viewModel.errorMessage(),
             QStringLiteral("转出账户和转入账户不能相同"));
    QVERIFY(transactions.findAll()->empty());
}

QTEST_APPLESS_MAIN(TransferFundsPresentationTest)

#include "TransferFundsPresentationTest.moc"
