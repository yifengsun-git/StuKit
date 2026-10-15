#include <QCoreApplication>
#include <QColor>
#include <QDebug>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTest>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

namespace {

QVariantMap overviewModel()
{
    return {
        {QStringLiteral("hasError"), false},
        {QStringLiteral("errorMessage"), QString{}},
        {QStringLiteral("monthLabel"), QStringLiteral("2026 年 10 月")},
        {QStringLiteral("timeZoneId"), QStringLiteral("Asia/Shanghai")},
        {QStringLiteral("totalIncomeText"), QStringLiteral("0.00")},
        {QStringLiteral("totalExpenseText"), QStringLiteral("0.00")},
        {QStringLiteral("balanceText"), QStringLiteral("0.00")},
        {QStringLiteral("empty"), true},
        {QStringLiteral("typeFilter"), 0},
        {QStringLiteral("accountIdFilter"), QString{}},
        {QStringLiteral("categoryIdFilter"), QString{}},
        {QStringLiteral("searchText"), QString{}},
        {QStringLiteral("noMatchingTransactions"), false},
        {QStringLiteral("filteredTransactionCount"), 0},
        {QStringLiteral("hasActiveTransactionFilters"), false},
        {QStringLiteral("accountFilterOptions"), QVariantList{}},
        {QStringLiteral("categoryFilterOptions"), QVariantList{}},
        {QStringLiteral("transactions"), QVariantList{}},
        {QStringLiteral("availableTimeZoneIds"),
         QVariantList{QStringLiteral("Asia/Shanghai")}},
    };
}

std::unique_ptr<QObject> createPage(
    QQmlEngine &engine,
    qreal width,
    qreal height = 720)
{
    QQmlComponent component(&engine);
    component.loadFromModule(
        QStringLiteral("StuKitAccountingHomeTest"),
        QStringLiteral("AccountingHomePage"));
    if (!component.isReady()) {
        qWarning().noquote() << component.errorString();
    }

    const QVariantMap balances{
        {QStringLiteral("hasError"), false},
        {QStringLiteral("errorMessage"), QString{}},
        {QStringLiteral("activeTotalText"), QStringLiteral("0.00")},
        {QStringLiteral("accounts"), QVariantList{}},
    };
    const QVariantMap setup{
        {QStringLiteral("hasError"), false},
        {QStringLiteral("errorMessage"), QString{}},
        {QStringLiteral("hasAccounts"), true},
        {QStringLiteral("hasIncomeCategories"), true},
        {QStringLiteral("hasExpenseCategories"), true},
        {QStringLiteral("accounts"), QVariantList{}},
        {QStringLiteral("categories"), QVariantList{}},
    };
    const QVariantMap transactionEditor{
        {QStringLiteral("ready"), true},
        {QStringLiteral("editing"), false},
        {QStringLiteral("income"), false},
        {QStringLiteral("availableAccounts"), QVariantList{}},
        {QStringLiteral("availableCategories"), QVariantList{}},
        {QStringLiteral("hasError"), false},
        {QStringLiteral("errorMessage"), QString{}},
        {QStringLiteral("canSubmit"), false},
        {QStringLiteral("submitButtonText"), QStringLiteral("保存")},
    };
    const QVariantMap maintenance{
        {QStringLiteral("busy"), false},
        {QStringLiteral("hasError"), false},
        {QStringLiteral("errorMessage"), QString{}},
        {QStringLiteral("statusMessage"), QString{}},
    };

    QVariantMap properties{
        {QStringLiteral("viewModel"), overviewModel()},
        {QStringLiteral("balancesViewModel"), balances},
        {QStringLiteral("setupViewModel"), setup},
        {QStringLiteral("transactionViewModel"), transactionEditor},
        {QStringLiteral("maintenanceViewModel"), maintenance},
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

} // namespace

class AccountingHomeQmlIntegrationTest final : public QObject
{
    Q_OBJECT

private slots:
    void adaptsSummaryAndPrimaryCardToAvailableWidth();
    void keepsDataToolsCollapsedUntilRequested();
    void constrainsLongDialogsToTheViewport();
    void usesDarkSelectedStateForManagementTabs();
};

void AccountingHomeQmlIntegrationTest::
    adaptsSummaryAndPrimaryCardToAvailableWidth()
{
    QQmlEngine engine;
    auto page = createPage(engine, 360, 640);
    QVERIFY(page);

    QObject *primaryCard = page->findChild<QObject *>(
        QStringLiteral("accountingPrimaryCard"));
    QObject *summaryGrid = page->findChild<QObject *>(
        QStringLiteral("accountingSummaryGrid"));
    QVERIFY(primaryCard);
    QVERIFY(summaryGrid);

    QCOMPARE(page->property("compactLayout").toBool(), true);
    QTRY_COMPARE(summaryGrid->property("columns").toInt(), 1);
    QTRY_VERIFY(primaryCard->property("width").toReal() > 0.0);
    QVERIFY(primaryCard->property("width").toReal() <= 360.0);

    page->setProperty("width", 960.0);
    QCoreApplication::processEvents();

    QTRY_COMPARE(page->property("compactLayout").toBool(), false);
    QTRY_COMPARE(summaryGrid->property("columns").toInt(), 4);
    QVERIFY(primaryCard->property("width").toReal() <= 960.0);
}

void AccountingHomeQmlIntegrationTest::keepsDataToolsCollapsedUntilRequested()
{
    QQmlEngine engine;
    auto page = createPage(engine, 360, 640);
    QVERIFY(page);

    QObject *dataToolsCard = page->findChild<QObject *>(
        QStringLiteral("accountingDataToolsCard"));
    QObject *dataToolsFlow = page->findChild<QObject *>(
        QStringLiteral("accountingDataToolsFlow"));
    QVERIFY(dataToolsCard);
    QVERIFY(dataToolsFlow);
    QCOMPARE(page->property("dataToolsExpanded").toBool(), false);
    QCOMPARE(dataToolsFlow->property("visible").toBool(), false);

    const qreal collapsedHeight
        = dataToolsCard->property("implicitHeight").toReal();
    page->setProperty("dataToolsExpanded", true);
    QCoreApplication::processEvents();

    QTRY_COMPARE(dataToolsFlow->property("visible").toBool(), true);
    QTRY_VERIFY(dataToolsCard->property("implicitHeight").toReal()
        > collapsedHeight);
}

void AccountingHomeQmlIntegrationTest::constrainsLongDialogsToTheViewport()
{
    QQmlEngine engine;
    auto page = createPage(engine, 360, 480);
    QVERIFY(page);

    QObject *transactionDialog = page->findChild<QObject *>(
        QStringLiteral("accountingTransactionDialog"));
    QObject *setupDialog = page->findChild<QObject *>(
        QStringLiteral("accountingSetupManagementDialog"));
    QVERIFY(transactionDialog);
    QVERIFY(setupDialog);

    for (QObject *dialog : {transactionDialog, setupDialog}) {
        QVERIFY(dialog->property("width").toReal() <= 328.0);
        QVERIFY(dialog->property("height").toReal() <= 448.0);
        QCOMPARE(dialog->property("maximumDialogHeight").toReal(), 448.0);
    }

    QTRY_COMPARE(
        transactionDialog->property("contentScrollable").toBool(), true);
}

void AccountingHomeQmlIntegrationTest::
    usesDarkSelectedStateForManagementTabs()
{
    QQmlEngine engine;
    auto page = createPage(engine, 640, 720);
    QVERIFY(page);

    QObject *accounts = page->findChild<QObject *>(
        QStringLiteral("managedAccountsTab"));
    QObject *categories = page->findChild<QObject *>(
        QStringLiteral("managedCategoriesTab"));
    QObject *accountsBackground = page->findChild<QObject *>(
        QStringLiteral("managedAccountsTabBackground"));
    QObject *categoriesBackground = page->findChild<QObject *>(
        QStringLiteral("managedCategoriesTabBackground"));
    QVERIFY(accounts);
    QVERIFY(categories);
    QVERIFY(accountsBackground);
    QVERIFY(categoriesBackground);

    QCOMPARE(accounts->property("checked").toBool(), true);
    QCOMPARE(accountsBackground->property("color").value<QColor>(),
             QColor(QStringLiteral("#171717")));

    QVERIFY(QMetaObject::invokeMethod(categories, "click"));
    QCoreApplication::processEvents();
    QTRY_COMPARE(categories->property("checked").toBool(), true);
    QTRY_COMPARE(categoriesBackground->property("color").value<QColor>(),
                 QColor(QStringLiteral("#171717")));
    QCOMPARE(accounts->property("checked").toBool(), false);
}

QTEST_MAIN(AccountingHomeQmlIntegrationTest)

#include "AccountingHomeQmlIntegrationTest.moc"
