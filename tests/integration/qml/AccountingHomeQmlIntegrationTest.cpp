#include <QCoreApplication>
#include <QAbstractListModel>
#include <QColor>
#include <QDebug>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

namespace {

class FakeTransactionModel final : public QAbstractListModel
{
public:
    enum Role
    {
        TransactionIdRole = Qt::UserRole + 1,
        IsIncomeRole,
        AmountTextRole,
        AccountNameRole,
        CategoryNameRole,
        TypeTextRole,
        OccurredAtTextRole,
        FullOccurredAtTextRole,
        NoteRole,
    };

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : 1;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || index.row() != 0) {
            return {};
        }
        switch (role) {
        case TransactionIdRole:
            return QStringLiteral("transaction-1");
        case IsIncomeRole:
            return false;
        case AmountTextRole:
            return QStringLiteral("12.50");
        case AccountNameRole:
            return QStringLiteral("现金");
        case CategoryNameRole:
            return QStringLiteral("餐饮");
        case TypeTextRole:
            return QStringLiteral("支出");
        case OccurredAtTextRole:
            return QStringLiteral("11-01 12:30");
        case FullOccurredAtTextRole:
            return QStringLiteral("2026-11-01 12:30");
        case NoteRole:
            return QStringLiteral("午餐");
        default:
            return {};
        }
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {TransactionIdRole, QByteArrayLiteral("transactionId")},
            {IsIncomeRole, QByteArrayLiteral("isIncome")},
            {AmountTextRole, QByteArrayLiteral("amountText")},
            {AccountNameRole, QByteArrayLiteral("accountName")},
            {CategoryNameRole, QByteArrayLiteral("categoryName")},
            {TypeTextRole, QByteArrayLiteral("typeText")},
            {OccurredAtTextRole, QByteArrayLiteral("occurredAtText")},
            {FullOccurredAtTextRole, QByteArrayLiteral("fullOccurredAtText")},
            {NoteRole, QByteArrayLiteral("note")},
        };
    }
};

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
        {QStringLiteral("calendarDays"), QVariantList{}},
        {QStringLiteral("selectedCalendarDate"), QString{}},
        {QStringLiteral("dailyTransactions"), QVariantList{}},
        {QStringLiteral("transactions"), QVariantList{}},
        {QStringLiteral("availableTimeZoneIds"),
         QVariantList{QStringLiteral("Asia/Shanghai")}},
    };
}

std::unique_ptr<QObject> createPage(
    QQmlEngine &engine,
    qreal width,
    qreal height = 720,
    QVariantMap overview = {})
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
    const QVariantMap transferEditor{
        {QStringLiteral("ready"), true},
        {QStringLiteral("availableAccounts"), QVariantList{}},
        {QStringLiteral("incomeCategories"), QVariantList{}},
        {QStringLiteral("expenseCategories"), QVariantList{}},
        {QStringLiteral("occurredDateText"), QStringLiteral("2026-11-01")},
        {QStringLiteral("occurredTimeText"), QStringLiteral("12:00")},
        {QStringLiteral("hasError"), false},
        {QStringLiteral("errorMessage"), QString{}},
    };

    if (overview.isEmpty()) {
        overview = overviewModel();
    }
    QVariantMap properties{
        {QStringLiteral("viewModel"), overview},
        {QStringLiteral("balancesViewModel"), balances},
        {QStringLiteral("setupViewModel"), setup},
        {QStringLiteral("transactionViewModel"), transactionEditor},
        {QStringLiteral("transferViewModel"), transferEditor},
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
    void exposesTransferAndRollingDateControls();
    void rendersRealTransactionDelegates();
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

void AccountingHomeQmlIntegrationTest::exposesTransferAndRollingDateControls()
{
    QQmlEngine engine;
    auto page = createPage(engine, 640, 720);
    QVERIFY(page);

    QObject *transferButton = page->findChild<QObject *>(
        QStringLiteral("accountingTransferButton"));
    QObject *transactionDialog = page->findChild<QObject *>(
        QStringLiteral("accountingTransactionDialog"));
    QObject *transferDialog = page->findChild<QObject *>(
        QStringLiteral("accountingTransferDialog"));
    QObject *dailyTransactionsDialog = page->findChild<QObject *>(
        QStringLiteral("accountingDailyTransactionsDialog"));
    QObject *dailyTransactionList = page->findChild<QObject *>(
        QStringLiteral("accountingDailyTransactionList"));
    QObject *dateField = page->findChild<QObject *>(
        QStringLiteral("accountingTransactionDateField"));
    QObject *pickerButton = page->findChild<QObject *>(
        QStringLiteral("accountingTransactionDatePickerButton"));
    QObject *datePicker = page->findChild<QObject *>(
        QStringLiteral("accountingTransactionDateTimePicker"));
    QObject *yearTumbler = page->findChild<QObject *>(
        QStringLiteral("datePickerYearTumbler"));
    QObject *monthTumbler = page->findChild<QObject *>(
        QStringLiteral("datePickerMonthTumbler"));
    QObject *dayTumbler = page->findChild<QObject *>(
        QStringLiteral("datePickerDayTumbler"));
    QVERIFY(transferButton);
    QVERIFY(transactionDialog);
    QVERIFY(transferDialog);
    QVERIFY(dailyTransactionsDialog);
    QVERIFY(dailyTransactionList);
    QVERIFY(dateField);
    QVERIFY(pickerButton);
    QVERIFY(datePicker);
    QVERIFY(yearTumbler);
    QVERIFY(monthTumbler);
    QVERIFY(dayTumbler);
    QCOMPARE(dateField->property("readOnly").toBool(), false);
    QCOMPARE(datePicker->property("includeTime").toBool(), false);
    QCOMPARE(transactionDialog->property("enterActionEnabled").toBool(), true);
    QCOMPARE(transferDialog->property("enterActionEnabled").toBool(), true);
}

void AccountingHomeQmlIntegrationTest::rendersRealTransactionDelegates()
{
    QQmlEngine engine;
    QQuickWindow window;
    QStringList qmlWarnings;
    connect(
        &engine,
        &QQmlEngine::warnings,
        this,
        [&qmlWarnings](const QList<QQmlError> &warnings) {
            for (const QQmlError &warning : warnings) {
                qmlWarnings.push_back(warning.toString());
            }
        });
    FakeTransactionModel transactions;
    QVariantMap overview = overviewModel();
    overview.insert(QStringLiteral("empty"), false);
    overview.insert(QStringLiteral("filteredTransactionCount"), 1);
    overview.insert(
        QStringLiteral("transactions"),
        QVariant::fromValue(static_cast<QObject *>(&transactions)));
    overview.insert(
        QStringLiteral("dailyTransactions"),
        QVariant::fromValue(static_cast<QObject *>(&transactions)));

    auto page = createPage(engine, 640, 720, overview);
    QVERIFY(page);
    auto *pageItem = qobject_cast<QQuickItem *>(page.get());
    QVERIFY(pageItem);
    pageItem->setParentItem(window.contentItem());
    window.resize(640, 720);
    window.show();
    page->setProperty("transactionDisplayMode", QStringLiteral("list"));
    QCoreApplication::processEvents();

    QObject *monthlyList = page->findChild<QObject *>(
        QStringLiteral("accountingTransactionList"));
    QObject *dailyList = page->findChild<QObject *>(
        QStringLiteral("accountingDailyTransactionList"));
    QObject *dailyDialog = page->findChild<QObject *>(
        QStringLiteral("accountingDailyTransactionsDialog"));
    QVERIFY(monthlyList);
    QVERIFY(dailyList);
    QVERIFY(dailyDialog);
    QTRY_COMPARE(monthlyList->property("count").toInt(), 1);
    QTRY_COMPARE(dailyList->property("count").toInt(), 1);
    QTRY_VERIFY(monthlyList->property("contentHeight").toReal() >= 68.0);

    QVERIFY(QMetaObject::invokeMethod(dailyDialog, "open"));
    QCoreApplication::processEvents();
    QTRY_VERIFY(dailyList->property("contentHeight").toReal() >= 68.0);
    QVERIFY2(qmlWarnings.isEmpty(),
             qPrintable(qmlWarnings.join(QLatin1Char('\n'))));
}

QTEST_MAIN(AccountingHomeQmlIntegrationTest)

#include "AccountingHomeQmlIntegrationTest.moc"
