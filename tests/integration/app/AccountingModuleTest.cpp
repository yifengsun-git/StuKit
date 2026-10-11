#include "AccountingModule.h"
#include "AccountingOverviewViewModel.h"
#include "AccountingSetupViewModel.h"
#include "CreateTransactionViewModel.h"
#include "TransactionListModel.h"
#include "UserSettings.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

using namespace stukit::app;

namespace {

QString uniqueConnectionName()
{
    return QStringLiteral("stukit-module-test-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

} // namespace

class AccountingModuleTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void initializesDatabaseAndViewModel();
    void createsAndRestoresSetupData();
    void createsEditsDeletesAndRestoresTransaction();
    void defaultsInvalidTimeZoneToUtcPlusEight();
    void persistsSelectedFixedTimeZone();
    void rejectsEmptyApplicationDataDirectory();
};

void AccountingModuleTest::initTestCase()
{
    QVERIFY2(QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")),
             "QSQLITE driver is not available");
}

void AccountingModuleTest::initializesDatabaseAndViewModel()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-module-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    AccountingModule module(directory.filePath(QStringLiteral("app-data")),
                            settings);

    QVERIFY2(module.initialize(QDate(2026, 10, 1)),
             qPrintable(module.lastError()));
    QVERIFY(module.overviewViewModel() != nullptr);
    QVERIFY(module.setupViewModel() != nullptr);
    QVERIFY(module.transactionEditorViewModel() != nullptr);
    QVERIFY(QFileInfo::exists(module.databasePath()));
    QVERIFY(module.databasePath().startsWith(directory.path()));
    QCOMPARE(module.overviewViewModel()->monthLabel(),
             QStringLiteral("2026-10"));
    QVERIFY(module.overviewViewModel()->isEmpty());
    QVERIFY(!module.overviewViewModel()->hasError());
    QCOMPARE(module.setupViewModel()->accounts()->rowCount(), 0);
    QCOMPARE(module.setupViewModel()->categories()->rowCount(), 0);
    QVERIFY(!module.setupViewModel()->hasError());
    QVERIFY(!module.transactionEditorViewModel()->isReady());

    const QString connectionName = uniqueConnectionName();
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), connectionName);
        database.setDatabaseName(module.databasePath());
        QVERIFY2(database.open(), qPrintable(database.lastError().text()));

        QSqlQuery query(database);
        QVERIFY2(query.exec(QStringLiteral(
                     "SELECT COUNT(*), MAX(version) FROM schema_migrations")),
                 qPrintable(query.lastError().text()));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 3);
        QCOMPARE(query.value(1).toInt(), 3);
        database.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

void AccountingModuleTest::createsEditsDeletesAndRestoresTransaction()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-module-transaction-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    const QString appDataDirectory = directory.filePath(
        QStringLiteral("app-data"));

    {
        AccountingModule module(appDataDirectory, settings);
        QVERIFY2(module.initialize(QDate(2026, 10, 1)),
                 qPrintable(module.lastError()));
        auto *setup = module.setupViewModel();
        QVERIFY(setup->createAccount(
            QStringLiteral("银行卡"), QStringLiteral("100.00")));
        QVERIFY(setup->createCategory(QStringLiteral("餐饮"), false));

        auto *editor = module.transactionEditorViewModel();
        QVERIFY(editor != nullptr);
        QVERIFY(editor->isReady());
        editor->resetForNewTransaction();
        editor->setAmountText(QStringLiteral("25.50"));
        editor->setOccurredDateText(QStringLiteral("2026-10-10"));
        editor->setOccurredTimeText(QStringLiteral("12:34"));
        editor->setNote(QStringLiteral("午饭"));

        QVERIFY2(editor->submit(), qPrintable(editor->errorMessage()));
        QCOMPARE(module.overviewViewModel()->transactions()->rowCount(), 1);
        const QModelIndex transactionIndex =
            module.overviewViewModel()->transactions()->index(0, 0);
        QCOMPARE(module.overviewViewModel()->transactions()->data(
                     transactionIndex,
                     stukit::accounting::presentation::TransactionListModel::AccountNameRole)
                     .toString(),
                 QStringLiteral("银行卡"));
        QCOMPARE(module.overviewViewModel()->transactions()->data(
                     transactionIndex,
                     stukit::accounting::presentation::TransactionListModel::CategoryNameRole)
                     .toString(),
                 QStringLiteral("餐饮"));
        QCOMPARE(module.overviewViewModel()->totalExpenseText(),
                 QStringLiteral("25.50"));
        QCOMPARE(module.overviewViewModel()->balanceText(),
                 QStringLiteral("-25.50"));

        const QString transactionId =
            module.overviewViewModel()->transactions()->data(
                transactionIndex,
                stukit::accounting::presentation::TransactionListModel::IdRole)
                .toString();
        QVERIFY(editor->beginEditing(transactionId));
        editor->setAmountText(QStringLiteral("30.00"));
        editor->setNote(QStringLiteral("晚饭"));
        QVERIFY2(editor->submit(), qPrintable(editor->errorMessage()));
        QCOMPARE(module.overviewViewModel()->transactions()->rowCount(), 1);
        QCOMPARE(module.overviewViewModel()->totalExpenseText(),
                 QStringLiteral("30.00"));
    }

    {
        AccountingModule restored(appDataDirectory, settings);
        QVERIFY2(restored.initialize(QDate(2026, 10, 1)),
                 qPrintable(restored.lastError()));
        QVERIFY2(!restored.overviewViewModel()->hasError(),
                 qPrintable(restored.overviewViewModel()->errorMessage()));
        QCOMPARE(restored.overviewViewModel()->transactions()->rowCount(), 1);
        QCOMPARE(restored.overviewViewModel()->totalExpenseText(),
                 QStringLiteral("30.00"));
        QVERIFY(restored.transactionEditorViewModel()->isReady());

        auto *model = restored.overviewViewModel()->transactions();
        const QString transactionId = model->data(
            model->index(0, 0),
            stukit::accounting::presentation::TransactionListModel::IdRole)
            .toString();
        QVERIFY(restored.transactionEditorViewModel()->deleteTransaction(
            transactionId));
        QCOMPARE(model->rowCount(), 0);
        QCOMPARE(restored.overviewViewModel()->totalExpenseText(),
                 QStringLiteral("0.00"));
    }

    {
        AccountingModule restored(appDataDirectory, settings);
        QVERIFY2(restored.initialize(QDate(2026, 10, 1)),
                 qPrintable(restored.lastError()));
        QCOMPARE(restored.overviewViewModel()->transactions()->rowCount(), 0);
    }
}

void AccountingModuleTest::createsAndRestoresSetupData()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-module-setup-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    const QString appDataDirectory = directory.filePath(
        QStringLiteral("app-data"));

    {
        AccountingModule module(appDataDirectory, settings);
        QVERIFY2(module.initialize(QDate(2026, 10, 1)),
                 qPrintable(module.lastError()));
        auto *setup = module.setupViewModel();
        QVERIFY(setup != nullptr);
        QVERIFY(setup->createAccount(
            QStringLiteral("银行卡"), QStringLiteral("123.45")));
        QVERIFY(setup->createCategory(QStringLiteral("工资"), true));
        QVERIFY(setup->createCategory(QStringLiteral("餐饮"), false));
        QVERIFY(setup->hasAccounts());
        QVERIFY(setup->hasIncomeCategories());
        QVERIFY(setup->hasExpenseCategories());
    }

    {
        AccountingModule restored(appDataDirectory, settings);
        QVERIFY2(restored.initialize(QDate(2026, 10, 1)),
                 qPrintable(restored.lastError()));
        auto *setup = restored.setupViewModel();
        QVERIFY(setup != nullptr);
        QCOMPARE(setup->accounts()->rowCount(), 1);
        QCOMPARE(setup->categories()->rowCount(), 2);
        QVERIFY(setup->hasAccounts());
        QVERIFY(setup->hasIncomeCategories());
        QVERIFY(setup->hasExpenseCategories());
        QVERIFY(!setup->hasError());
    }
}

void AccountingModuleTest::defaultsInvalidTimeZoneToUtcPlusEight()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-settings-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    settings.setValue(QStringLiteral("accounting/timeZoneId"),
                      QByteArrayLiteral("Not/A-Time-Zone"));
    UserSettings userSettings(settings);

    const QTimeZone timeZone = userSettings.accountingTimeZone();

    QVERIFY(timeZone.isValid());
    QCOMPARE(timeZone.id(), QByteArrayLiteral("UTC+08:00"));
    QCOMPARE(settings.value(QStringLiteral("accounting/timeZoneId")).toByteArray(),
             QByteArrayLiteral("UTC+08:00"));
}

void AccountingModuleTest::persistsSelectedFixedTimeZone()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-settings-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);

    {
        AccountingModule module(directory.filePath(QStringLiteral("app-data")),
                                settings);
        QVERIFY2(module.initialize(QDate(2026, 10, 1)),
                 qPrintable(module.lastError()));
        QVERIFY(module.overviewViewModel()->selectFixedOffsetMinutes(-5 * 60));
        QCOMPARE(module.transactionEditorViewModel()->timeZone().id(),
                 QByteArrayLiteral("UTC-05:00"));
        settings.sync();
    }

    UserSettings restoredSettings(settings);
    QCOMPARE(restoredSettings.accountingTimeZone().id(),
             QByteArrayLiteral("UTC-05:00"));
}

void AccountingModuleTest::rejectsEmptyApplicationDataDirectory()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-settings-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    AccountingModule module(QString{}, settings);

    QVERIFY(!module.initialize(QDate(2026, 10, 1)));
    QVERIFY(module.overviewViewModel() == nullptr);
    QVERIFY(module.setupViewModel() == nullptr);
    QVERIFY(module.transactionEditorViewModel() == nullptr);
    QVERIFY(!module.lastError().isEmpty());
}

QTEST_GUILESS_MAIN(AccountingModuleTest)

#include "AccountingModuleTest.moc"
