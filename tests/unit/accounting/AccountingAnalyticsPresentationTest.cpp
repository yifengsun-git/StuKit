#include "AccountAnalyticsListModel.h"
#include "AccountingAnalyticsViewModel.h"
#include "CategoryAnalyticsListModel.h"
#include "ChartSeriesListModel.h"
#include "MonthlyAnalyticsListModel.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryCategoryRepository.h"
#include "support/InMemoryTransactionRepository.h"

#include <QSignalSpy>
#include <QTest>
#include <QVariantList>
#include <QVariantMap>

#include <limits>

using namespace stukit::accounting;
using namespace stukit::accounting::presentation;
using namespace stukit::accounting::testing;
using stukit::analytics::presentation::ChartSeriesListModel;

namespace {

QTimeZone utcPlusEight()
{
    return QTimeZone::fromSecondsAheadOfUtc(8 * 60 * 60);
}

Account makeAccount(const QUuid &id, const QString &name, bool archived = false)
{
    return *Account::create({
        .id = id,
        .name = name,
        .openingBalance = Money::fromCents(0),
        .archived = archived,
    });
}

Category makeCategory(
    const QUuid &id,
    const QString &name,
    TransactionType type,
    bool archived = false)
{
    return *Category::create({
        .id = id,
        .name = name,
        .type = type,
        .archived = archived,
    });
}

Transaction makeTransaction(
    TransactionType type,
    qint64 amountCents,
    const QString &utcTime,
    const QUuid &accountId,
    const QUuid &categoryId)
{
    return *Transaction::create({
        .id = QUuid::createUuid(),
        .type = type,
        .amount = Money::fromCents(amountCents),
        .accountId = accountId,
        .categoryId = categoryId,
        .occurredAt = QDateTime::fromString(utcTime, Qt::ISODateWithMs),
        .note = {},
    });
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
        , getSetup(accounts, categories)
        , viewModel(
              getAnalytics,
              getSetup,
              utcPlusEight(),
              QDate(2026, 10, 1))
    {
        const bool saved = accounts.save(makeAccount(
                               cashId, QStringLiteral("现金")))
            && accounts.save(makeAccount(
                cardId, QStringLiteral("旧银行卡"), true))
            && categories.save(makeCategory(
                salaryId, QStringLiteral("工资"), TransactionType::Income))
            && categories.save(makeCategory(
                diningId, QStringLiteral("餐饮"),
                TransactionType::Expense, true))
            && categories.save(makeCategory(
                transitId, QStringLiteral("交通"),
                TransactionType::Expense));
        Q_ASSERT(saved);
        Q_UNUSED(saved);
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
    GetAccountingSetup getSetup;
    AccountingAnalyticsViewModel viewModel;
};

} // namespace

class AccountingAnalyticsPresentationTest final : public QObject
{
    Q_OBJECT

private slots:
    void exposesDefaultsSummariesAndModels();
    void appliesAndResetsCustomFilters();
    void mapsErrorsAndClearsStaleData();
};

void AccountingAnalyticsPresentationTest::exposesDefaultsSummariesAndModels()
{
    Fixture fixture;
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Income, 5000,
        QStringLiteral("2026-09-10T01:00:00.000Z"),
        fixture.cashId, fixture.salaryId)));
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Income, 10000,
        QStringLiteral("2026-10-10T01:00:00.000Z"),
        fixture.cashId, fixture.salaryId)));
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Expense, 3000,
        QStringLiteral("2026-10-11T01:00:00.000Z"),
        fixture.cashId, fixture.diningId)));

    QSignalSpy analyticsSpy(
        &fixture.viewModel,
        &AccountingAnalyticsViewModel::analyticsChanged);
    fixture.viewModel.load();

    QCOMPARE(analyticsSpy.count(), 1);
    QCOMPARE(fixture.viewModel.monthLabel(), QStringLiteral("2026-10"));
    QCOMPARE(fixture.viewModel.monthCount(), 12);
    QVERIFY(fixture.viewModel.selectedAccountIds().isEmpty());
    QVERIFY(fixture.viewModel.selectedCategoryIds().isEmpty());
    QCOMPARE(fixture.viewModel.accountFilterOptions().size(), 2);
    QCOMPARE(fixture.viewModel.categoryFilterOptions().size(), 3);
    QCOMPARE(fixture.viewModel.months()->rowCount(), 12);
    QCOMPARE(fixture.viewModel.incomeCategories()->rowCount(), 1);
    QCOMPARE(fixture.viewModel.expenseCategories()->rowCount(), 1);
    QCOMPARE(fixture.viewModel.accountFlows()->rowCount(), 1);
    QCOMPARE(fixture.viewModel.trendSeries()->rowCount(), 3);

    QCOMPARE(fixture.viewModel.selectedIncomeText(), QStringLiteral("100.00"));
    QCOMPARE(fixture.viewModel.selectedExpenseText(), QStringLiteral("30.00"));
    QCOMPARE(fixture.viewModel.selectedBalanceText(), QStringLiteral("70.00"));
    QCOMPARE(fixture.viewModel.incomeChangeText(), QStringLiteral("+50.00"));
    QCOMPARE(fixture.viewModel.expenseChangeText(), QStringLiteral("+30.00"));
    QCOMPARE(fixture.viewModel.balanceChangeText(), QStringLiteral("+20.00"));
    QVERIFY(fixture.viewModel.hasData());
    QVERIFY(!fixture.viewModel.isLoading());
    QVERIFY(!fixture.viewModel.hasError());

    const QModelIndex monthIndex = fixture.viewModel.months()->index(11, 0);
    QCOMPARE(fixture.viewModel.months()->data(
                 monthIndex, MonthlyAnalyticsListModel::MonthLabelRole),
             QVariant(QStringLiteral("2026-10")));
    QCOMPARE(fixture.viewModel.months()->data(
                 monthIndex, MonthlyAnalyticsListModel::IncomeCentsRole),
             QVariant::fromValue<qint64>(10000));

    const QModelIndex categoryIndex =
        fixture.viewModel.expenseCategories()->index(0, 0);
    QCOMPARE(fixture.viewModel.expenseCategories()->data(
                 categoryIndex, CategoryAnalyticsListModel::NameRole),
             QVariant(QStringLiteral("餐饮")));
    QCOMPARE(fixture.viewModel.expenseCategories()->data(
                 categoryIndex,
                 CategoryAnalyticsListModel::PercentageTextRole),
             QVariant(QStringLiteral("100.0%")));
    QVERIFY(fixture.viewModel.expenseCategories()->data(
                categoryIndex, CategoryAnalyticsListModel::ArchivedRole)
                .toBool());

    const QModelIndex seriesIndex = fixture.viewModel.trendSeries()->index(0, 0);
    QCOMPARE(fixture.viewModel.trendSeries()->data(
                 seriesIndex, ChartSeriesListModel::SeriesIdRole),
             QVariant(QStringLiteral("income")));
    const QVariantList points = fixture.viewModel.trendSeries()->data(
        seriesIndex, ChartSeriesListModel::PointsRole).toList();
    QCOMPARE(points.size(), 12);
    QCOMPARE(points.back().toMap().value(QStringLiteral("displayValue")),
             QVariant(QStringLiteral("100.00")));
}

void AccountingAnalyticsPresentationTest::appliesAndResetsCustomFilters()
{
    Fixture fixture;
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Expense, 500,
        QStringLiteral("2026-10-10T01:00:00.000Z"),
        fixture.cashId, fixture.diningId)));
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Expense, 900,
        QStringLiteral("2026-10-11T01:00:00.000Z"),
        fixture.cardId, fixture.diningId)));
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Expense, 300,
        QStringLiteral("2026-10-12T01:00:00.000Z"),
        fixture.cashId, fixture.transitId)));
    fixture.viewModel.load();

    QVERIFY(fixture.viewModel.setAccountSelected(
        fixture.cashId.toString(QUuid::WithoutBraces), true));
    QCOMPARE(fixture.viewModel.selectedExpenseText(), QStringLiteral("8.00"));
    QCOMPARE(fixture.viewModel.selectedAccountIds().size(), 1);

    QVERIFY(fixture.viewModel.setCategorySelected(
        fixture.diningId.toString(QUuid::WithoutBraces), true));
    QCOMPARE(fixture.viewModel.selectedExpenseText(), QStringLiteral("5.00"));
    QCOMPARE(fixture.viewModel.selectedCategoryIds().size(), 1);

    fixture.viewModel.setMonthCount(6);
    QCOMPARE(fixture.viewModel.monthCount(), 6);
    QCOMPARE(fixture.viewModel.months()->rowCount(), 6);
    fixture.viewModel.showPreviousMonth();
    QCOMPARE(fixture.viewModel.monthLabel(), QStringLiteral("2026-09"));

    fixture.viewModel.setMonthCount(1);
    QCOMPARE(fixture.viewModel.monthCount(), 6);
    QCOMPARE(fixture.viewModel.errorMessage(),
             QStringLiteral("分析条件无效，请恢复默认条件"));
    QVERIFY(!fixture.viewModel.setAccountSelected(
        QUuid::createUuid().toString(QUuid::WithoutBraces), true));

    fixture.viewModel.resetFilters();
    QCOMPARE(fixture.viewModel.monthLabel(), QStringLiteral("2026-10"));
    QCOMPARE(fixture.viewModel.monthCount(), 12);
    QVERIFY(fixture.viewModel.selectedAccountIds().isEmpty());
    QVERIFY(fixture.viewModel.selectedCategoryIds().isEmpty());
    QCOMPARE(fixture.viewModel.selectedExpenseText(), QStringLiteral("17.00"));
    QVERIFY(!fixture.viewModel.hasError());
}

void AccountingAnalyticsPresentationTest::mapsErrorsAndClearsStaleData()
{
    Fixture fixture;
    QVERIFY(fixture.transactions.save(makeTransaction(
        TransactionType::Income, 100,
        QStringLiteral("2026-10-10T01:00:00.000Z"),
        fixture.cashId, fixture.salaryId)));
    fixture.viewModel.load();
    QCOMPARE(fixture.viewModel.months()->rowCount(), 12);

    fixture.transactions.setFindOccurredBetweenShouldSucceed(false);
    fixture.viewModel.load();
    QCOMPARE(fixture.viewModel.errorMessage(),
             QStringLiteral("暂时无法读取记账数据"));
    QCOMPARE(fixture.viewModel.months()->rowCount(), 0);
    QCOMPARE(fixture.viewModel.trendSeries()->rowCount(), 0);
    QVERIFY(!fixture.viewModel.hasData());

    Fixture overflowFixture;
    QVERIFY(overflowFixture.transactions.save(makeTransaction(
        TransactionType::Income,
        std::numeric_limits<qint64>::max(),
        QStringLiteral("2026-10-10T01:00:00.000Z"),
        overflowFixture.cashId, overflowFixture.salaryId)));
    QVERIFY(overflowFixture.transactions.save(makeTransaction(
        TransactionType::Income, 1,
        QStringLiteral("2026-10-11T01:00:00.000Z"),
        overflowFixture.cashId, overflowFixture.salaryId)));
    overflowFixture.viewModel.load();
    QCOMPARE(overflowFixture.viewModel.errorMessage(),
             QStringLiteral("金额过大，无法安全完成统计"));
    QCOMPARE(overflowFixture.viewModel.months()->rowCount(), 0);
}

QTEST_APPLESS_MAIN(AccountingAnalyticsPresentationTest)

#include "AccountingAnalyticsPresentationTest.moc"
