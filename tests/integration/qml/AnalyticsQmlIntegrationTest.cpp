#include "AccountingAnalyticsViewModel.h"
#include "ChartSeriesListModel.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryCategoryRepository.h"
#include "support/InMemoryTransactionRepository.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QDebug>
#include <QSignalSpy>
#include <QTest>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

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
    explicit Fixture(bool seedTransactions)
        : cashId(QUuid::createUuid())
        , archivedCardId(QUuid::createUuid())
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
        const bool setupSaved = accounts.save(makeAccount(
                                    cashId, QStringLiteral("现金")))
            && accounts.save(makeAccount(
                archivedCardId, QStringLiteral("旧银行卡"), true))
            && categories.save(makeCategory(
                salaryId, QStringLiteral("工资"), TransactionType::Income))
            && categories.save(makeCategory(
                diningId, QStringLiteral("餐饮"),
                TransactionType::Expense, true))
            && categories.save(makeCategory(
                transitId, QStringLiteral("交通"),
                TransactionType::Expense));
        Q_ASSERT(setupSaved);
        Q_UNUSED(setupSaved);

        if (seedTransactions) {
            const bool transactionsSaved = transactions.save(makeTransaction(
                                               TransactionType::Income, 5000,
                                               QStringLiteral(
                                                   "2025-12-31T16:30:00.000Z"),
                                               cashId, salaryId))
                && transactions.save(makeTransaction(
                    TransactionType::Expense, 75,
                    QStringLiteral("2026-09-05T01:00:00.000Z"),
                    cashId, transitId))
                && transactions.save(makeTransaction(
                    TransactionType::Income, 10000,
                    QStringLiteral("2026-10-10T01:00:00.000Z"),
                    cashId, salaryId))
                && transactions.save(makeTransaction(
                    TransactionType::Expense, 12000,
                    QStringLiteral("2026-10-11T01:00:00.000Z"),
                    archivedCardId, diningId));
            Q_ASSERT(transactionsSaved);
            Q_UNUSED(transactionsSaved);
        }

        viewModel.load();
    }

    InMemoryTransactionRepository transactions;
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    QUuid cashId;
    QUuid archivedCardId;
    QUuid salaryId;
    QUuid diningId;
    QUuid transitId;
    GetAccountingAnalytics getAnalytics;
    GetAccountingSetup getSetup;
    AccountingAnalyticsViewModel viewModel;
};

std::unique_ptr<QObject> createAnalyticsPage(
    QQmlEngine &engine,
    AccountingAnalyticsViewModel &viewModel,
    qreal width = 960,
    qreal height = 640)
{
    QQmlComponent component(&engine);
    component.loadFromModule(
        QStringLiteral("StuKitAnalyticsTest"),
        QStringLiteral("AnalyticsHomePage"));
    if (!component.isReady()) {
        qWarning().noquote() << component.errorString();
    }

    QVariantMap properties{
        {QStringLiteral("viewModel"),
         QVariant::fromValue(static_cast<QObject *>(&viewModel))},
        {QStringLiteral("startupError"), QString{}},
    };
    std::unique_ptr<QObject> page(
        component.createWithInitialProperties(properties));
    if (!page) {
        qWarning().noquote() << component.errorString();
        return {};
    }
    page->setProperty("width", width);
    page->setProperty("height", height);
    QCoreApplication::processEvents();
    return page;
}

int pointCount(QAbstractItemModel *model, int seriesRow)
{
    const QVariantList points = model->data(
        model->index(seriesRow, 0),
        ChartSeriesListModel::PointsRole).toList();
    return static_cast<int>(points.size());
}

} // namespace

class AnalyticsQmlIntegrationTest final : public QObject
{
    Q_OBJECT

private slots:
    void rendersEmptyStatesAndResponsiveFilter();
    void switchesChartsAndTracksMonthRange();
    void updatesFilterSummaryAndRestoresDefaults();
    void rebindsReplacementViewModel();
    void requestsReturnToAccounting();
};

void AnalyticsQmlIntegrationTest::rendersEmptyStatesAndResponsiveFilter()
{
    Fixture fixture(false);
    QQmlEngine engine;
    auto page = createAnalyticsPage(engine, fixture.viewModel, 640, 480);
    QVERIFY(page);

    QObject *filter = page->findChild<QObject *>(
        QStringLiteral("analyticsFilterPanel"));
    QObject *barChart = page->findChild<QObject *>(
        QStringLiteral("monthlyBarChart"));
    QObject *balanceChart = page->findChild<QObject *>(
        QStringLiteral("balanceLineChart"));
    QObject *expenseRanking = page->findChild<QObject *>(
        QStringLiteral("expenseRankingBars"));
    QObject *expenseDonut = page->findChild<QObject *>(
        QStringLiteral("expenseDonutChart"));
    QObject *summaryGrid = page->findChild<QObject *>(
        QStringLiteral("analyticsSummaryGrid"));
    QObject *trendGrid = page->findChild<QObject *>(
        QStringLiteral("analyticsTrendGrid"));
    QObject *categoryGrid = page->findChild<QObject *>(
        QStringLiteral("analyticsCategoryGrid"));
    QObject *rangeBox = page->findChild<QObject *>(
        QStringLiteral("analyticsRangeBox"));

    QVERIFY(filter);
    QVERIFY(barChart);
    QVERIFY(balanceChart);
    QVERIFY(expenseRanking);
    QVERIFY(expenseDonut);
    QVERIFY(summaryGrid);
    QVERIFY(trendGrid);
    QVERIFY(categoryGrid);
    QVERIFY(rangeBox);
    QVERIFY(!filter->property("expanded").toBool());
    QCOMPARE(summaryGrid->property("columns").toInt(), 1);
    QCOMPARE(trendGrid->property("columns").toInt(), 1);
    QCOMPARE(categoryGrid->property("columns").toInt(), 1);
    QVERIFY(rangeBox->property("height").toReal() >= 44.0);
    QVERIFY(!barChart->property("hasValues").toBool());
    QVERIFY(!balanceChart->property("hasValues").toBool());
    QVERIFY(expenseRanking->findChild<QObject *>(
        QStringLiteral("rankingEmptyState"))->property("visible").toBool());
    QVERIFY(!expenseDonut->property("hasData").toBool());

    page->setProperty("width", 960);
    QTRY_VERIFY(filter->property("expanded").toBool());
    QTRY_COMPARE(summaryGrid->property("columns").toInt(), 3);
    QCOMPARE(trendGrid->property("columns").toInt(), 1);
    QCOMPARE(categoryGrid->property("columns").toInt(), 1);

    page->setProperty("width", 1280);
    QTRY_COMPARE(trendGrid->property("columns").toInt(), 2);
    QTRY_COMPARE(categoryGrid->property("columns").toInt(), 2);
}

void AnalyticsQmlIntegrationTest::switchesChartsAndTracksMonthRange()
{
    Fixture fixture(true);
    QQmlEngine engine;
    auto page = createAnalyticsPage(engine, fixture.viewModel);
    QVERIFY(page);

    QObject *trendStack = page->findChild<QObject *>(
        QStringLiteral("trendChartStack"));
    QObject *expenseStack = page->findChild<QObject *>(
        QStringLiteral("expenseChartStack"));
    QObject *barChart = page->findChild<QObject *>(
        QStringLiteral("monthlyBarChart"));
    QObject *balanceChart = page->findChild<QObject *>(
        QStringLiteral("balanceLineChart"));
    QVERIFY(trendStack);
    QVERIFY(expenseStack);
    QVERIFY(barChart);
    QVERIFY(balanceChart);
    QVERIFY(barChart->property("hasValues").toBool());
    QVERIFY(balanceChart->property("hasValues").toBool());

    QCOMPARE(trendStack->property("currentIndex").toInt(), 0);
    page->setProperty("trendChartMode", 1);
    QTRY_COMPARE(trendStack->property("currentIndex").toInt(), 1);
    page->setProperty("expenseChartMode", 1);
    QTRY_COMPARE(expenseStack->property("currentIndex").toInt(), 1);

    fixture.viewModel.setMonthCount(6);
    QTRY_COMPARE(pointCount(fixture.viewModel.trendSeries(), 0), 6);
    fixture.viewModel.setMonthCount(24);
    QTRY_COMPARE(pointCount(fixture.viewModel.trendSeries(), 0), 24);
    QVERIFY(barChart->property("hasValues").toBool());

    page->setProperty("width", 640);
    QTRY_COMPARE(page->property("accountFlowColumns").toInt(), 1);
    page->setProperty("width", 960);
    QTRY_COMPARE(page->property("accountFlowColumns").toInt(), 4);
}

void AnalyticsQmlIntegrationTest::updatesFilterSummaryAndRestoresDefaults()
{
    Fixture fixture(true);
    QQmlEngine engine;
    auto page = createAnalyticsPage(engine, fixture.viewModel);
    QVERIFY(page);

    QObject *summary = page->findChild<QObject *>(
        QStringLiteral("analyticsFilterSummary"));
    QVERIFY(summary);
    QVERIFY(summary->property("text").toString().contains(
        QStringLiteral("账户 不限")));
    QVERIFY(summary->property("text").toString().contains(
        QStringLiteral("分类 不限")));

    QVERIFY(fixture.viewModel.setAccountSelected(
        fixture.cashId.toString(QUuid::WithoutBraces), true));
    QVERIFY(fixture.viewModel.setCategorySelected(
        fixture.salaryId.toString(QUuid::WithoutBraces), true));
    QTRY_VERIFY(summary->property("text").toString().contains(
        QStringLiteral("账户 1 项")));
    QTRY_VERIFY(summary->property("text").toString().contains(
        QStringLiteral("分类 1 项")));

    fixture.viewModel.showPreviousMonth();
    fixture.viewModel.setMonthCount(6);
    fixture.viewModel.resetFilters();
    QTRY_COMPARE(fixture.viewModel.monthLabel(), QStringLiteral("2026-10"));
    QCOMPARE(fixture.viewModel.monthCount(), 12);
    QVERIFY(fixture.viewModel.selectedAccountIds().isEmpty());
    QVERIFY(fixture.viewModel.selectedCategoryIds().isEmpty());
    QTRY_VERIFY(summary->property("text").toString().contains(
        QStringLiteral("账户 不限")));
}

void AnalyticsQmlIntegrationTest::rebindsReplacementViewModel()
{
    Fixture populatedFixture(true);
    Fixture emptyFixture(false);
    QQmlEngine engine;
    auto page = createAnalyticsPage(engine, populatedFixture.viewModel);
    QVERIFY(page);

    QObject *filter = page->findChild<QObject *>(
        QStringLiteral("analyticsFilterPanel"));
    QObject *barChart = page->findChild<QObject *>(
        QStringLiteral("monthlyBarChart"));
    QVERIFY(filter);
    QVERIFY(barChart);
    QVERIFY(barChart->property("hasValues").toBool());

    page->setProperty(
        "viewModel",
        QVariant::fromValue(static_cast<QObject *>(&emptyFixture.viewModel)));
    QCoreApplication::processEvents();

    QCOMPARE(
        filter->property("viewModel").value<QObject *>(),
        static_cast<QObject *>(&emptyFixture.viewModel));
    QTRY_VERIFY(!barChart->property("hasValues").toBool());
}

void AnalyticsQmlIntegrationTest::requestsReturnToAccounting()
{
    Fixture fixture(false);
    QQmlEngine engine;
    auto page = createAnalyticsPage(engine, fixture.viewModel);
    QVERIFY(page);
    QObject *button = page->findChild<QObject *>(
        QStringLiteral("analyticsBackButton"));
    QVERIFY(button);
    QSignalSpy spy(page.get(), SIGNAL(backRequested()));
    QVERIFY(spy.isValid());
    QVERIFY(QMetaObject::invokeMethod(button, "click"));
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(AnalyticsQmlIntegrationTest)

#include "AnalyticsQmlIntegrationTest.moc"
