#include "TransactionFilterProxyModel.h"
#include "TransactionListModel.h"

#include <QTest>

using namespace stukit::accounting;
using namespace stukit::accounting::presentation;

namespace {

MonthlyTransactionItem makeItem(
    TransactionType type,
    const QUuid &accountId,
    const QString &accountName,
    const QUuid &categoryId,
    const QString &categoryName,
    const QString &note,
    int minute)
{
    const auto transaction = Transaction::create(TransactionData{
        .id = QUuid::createUuid(),
        .type = type,
        .amount = Money::fromCents(100 + minute),
        .accountId = accountId,
        .categoryId = categoryId,
        .occurredAt = QDateTime(
            QDate(2026, 10, 10), QTime(12, minute), QTimeZone::UTC),
        .note = note,
    });
    Q_ASSERT(transaction.has_value());
    return MonthlyTransactionItem{
        .transaction = *transaction,
        .accountName = accountName,
        .categoryName = categoryName,
    };
}

struct Fixture
{
    Fixture()
    {
        proxy.setSourceModel(&source);
        source.replaceTransactions({
            makeItem(
                TransactionType::Income,
                accountA,
                QStringLiteral("Cash Wallet"),
                incomeCategory,
                QStringLiteral("Salary"),
                QStringLiteral("Annual BONUS"),
                1),
            makeItem(
                TransactionType::Expense,
                accountA,
                QStringLiteral("Cash Wallet"),
                foodCategory,
                QStringLiteral("Food"),
                QStringLiteral("Lunch"),
                2),
            makeItem(
                TransactionType::Expense,
                archivedAccount,
                QStringLiteral("旧校园卡"),
                travelCategory,
                QStringLiteral("公交"),
                QStringLiteral("Dinner ride"),
                3),
        });
    }

    TransactionListModel source;
    TransactionFilterProxyModel proxy;
    QUuid accountA{QUuid::createUuid()};
    QUuid archivedAccount{QUuid::createUuid()};
    QUuid incomeCategory{QUuid::createUuid()};
    QUuid foodCategory{QUuid::createUuid()};
    QUuid travelCategory{QUuid::createUuid()};
};

QString idText(const QUuid &id)
{
    return id.toString(QUuid::WithoutBraces);
}

} // namespace

class TransactionFilterProxyModelTest final : public QObject
{
    Q_OBJECT

private slots:
    void filtersByTypeAccountAndCategory();
    void searchesNamesAndNotesCaseInsensitively();
    void combinesAndClearsFilters();
    void updatesAfterSourceReset();
};

void TransactionFilterProxyModelTest::filtersByTypeAccountAndCategory()
{
    Fixture fixture;
    QCOMPARE(fixture.proxy.rowCount(), 3);

    fixture.proxy.setTypeFilter(
        TransactionFilterProxyModel::IncomeTransactions);
    QCOMPARE(fixture.proxy.rowCount(), 1);
    QVERIFY(fixture.proxy.data(
        fixture.proxy.index(0, 0), TransactionListModel::IsIncomeRole).toBool());

    fixture.proxy.setTypeFilter(
        TransactionFilterProxyModel::ExpenseTransactions);
    QCOMPARE(fixture.proxy.rowCount(), 2);

    fixture.proxy.setAccountIdFilter(idText(fixture.archivedAccount));
    QCOMPARE(fixture.proxy.rowCount(), 1);
    QCOMPARE(fixture.proxy.data(
                 fixture.proxy.index(0, 0),
                 TransactionListModel::AccountNameRole).toString(),
             QStringLiteral("旧校园卡"));

    fixture.proxy.setCategoryIdFilter(idText(fixture.travelCategory));
    QCOMPARE(fixture.proxy.rowCount(), 1);
    fixture.proxy.setCategoryIdFilter(idText(fixture.foodCategory));
    QCOMPARE(fixture.proxy.rowCount(), 0);
}

void TransactionFilterProxyModelTest::searchesNamesAndNotesCaseInsensitively()
{
    Fixture fixture;

    fixture.proxy.setSearchText(QStringLiteral("  bonus  "));
    QCOMPARE(fixture.proxy.searchText(), QStringLiteral("bonus"));
    QCOMPARE(fixture.proxy.rowCount(), 1);

    fixture.proxy.setSearchText(QStringLiteral("cash wallet"));
    QCOMPARE(fixture.proxy.rowCount(), 2);

    fixture.proxy.setSearchText(QStringLiteral("公交"));
    QCOMPARE(fixture.proxy.rowCount(), 1);
}

void TransactionFilterProxyModelTest::combinesAndClearsFilters()
{
    Fixture fixture;

    fixture.proxy.setTypeFilter(
        TransactionFilterProxyModel::ExpenseTransactions);
    fixture.proxy.setAccountIdFilter(idText(fixture.archivedAccount));
    fixture.proxy.setCategoryIdFilter(idText(fixture.travelCategory));
    fixture.proxy.setSearchText(QStringLiteral("ride"));
    QVERIFY(fixture.proxy.hasActiveFilters());
    QCOMPARE(fixture.proxy.rowCount(), 1);

    fixture.proxy.setSearchText(QStringLiteral("lunch"));
    QCOMPARE(fixture.proxy.rowCount(), 0);

    fixture.proxy.clearFilters();
    QVERIFY(!fixture.proxy.hasActiveFilters());
    QCOMPARE(fixture.proxy.typeFilter(),
             TransactionFilterProxyModel::AllTransactions);
    QCOMPARE(fixture.proxy.rowCount(), 3);
}

void TransactionFilterProxyModelTest::updatesAfterSourceReset()
{
    Fixture fixture;
    fixture.proxy.setAccountIdFilter(idText(fixture.archivedAccount));
    QCOMPARE(fixture.proxy.rowCount(), 1);

    fixture.source.replaceTransactions({makeItem(
        TransactionType::Income,
        fixture.accountA,
        QStringLiteral("Cash Wallet"),
        fixture.incomeCategory,
        QStringLiteral("Salary"),
        QStringLiteral("New salary"),
        4)});

    QCOMPARE(fixture.proxy.rowCount(), 0);
    fixture.proxy.setAccountIdFilter(idText(fixture.accountA));
    QCOMPARE(fixture.proxy.rowCount(), 1);
}

QTEST_APPLESS_MAIN(TransactionFilterProxyModelTest)

#include "TransactionFilterProxyModelTest.moc"
