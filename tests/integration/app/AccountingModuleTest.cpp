#include "AccountingModule.h"
#include "AccountingUpdatePreparation.h"
#include "AccountBalanceListModel.h"
#include "AccountBalancesViewModel.h"
#include "AccountingAnalyticsViewModel.h"
#include "AccountingOverviewViewModel.h"
#include "AccountingSetupViewModel.h"
#include "AccountListModel.h"
#include "CategoryListModel.h"
#include "CreateTransactionViewModel.h"
#include "DataMaintenanceViewModel.h"
#include "TaskModule.h"
#include "TasksViewModel.h"
#include "TransactionListModel.h"
#include "UserSettings.h"
#include "backup/SqliteBackupService.h"

#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QSettings>
#include <QSignalSpy>
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
    void backsUpAndRestoresDatabase();
    void preparesDatabaseBackupBeforeUpdate();
    void exportsTransactionsThroughMaintenanceViewModel();
    void defaultsInvalidTimeZoneToUtcPlusEight();
    void persistsSelectedFixedTimeZone();
    void persistsTaskWorkflowAcrossRestart();
    void rejectsEmptyApplicationDataDirectory();
};

void AccountingModuleTest::initTestCase()
{
    QVERIFY2(QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")),
             "QSQLITE driver is not available");
}

void AccountingModuleTest::initializesDatabaseAndViewModel()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("stukit-module-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    AccountingModule module(directory.filePath(QStringLiteral("app-data")),
                            settings);

    QVERIFY2(module.initialize(QDate(2026, 10, 1)),
             qPrintable(module.lastError()));
    QVERIFY(module.overviewViewModel() != nullptr);
    QVERIFY(module.balancesViewModel() != nullptr);
    QVERIFY(module.analyticsViewModel() != nullptr);
    QVERIFY(module.setupViewModel() != nullptr);
    QVERIFY(module.transactionEditorViewModel() != nullptr);
    QVERIFY(QFileInfo::exists(module.databasePath()));
    QVERIFY(module.databasePath().startsWith(directory.path()));
    QCOMPARE(module.overviewViewModel()->monthLabel(),
             QStringLiteral("2026-10"));
    QVERIFY(module.overviewViewModel()->isEmpty());
    QVERIFY(!module.overviewViewModel()->hasError());
    QCOMPARE(module.balancesViewModel()->activeTotalText(),
             QStringLiteral("0.00"));
    QCOMPARE(module.analyticsViewModel()->monthCount(), 12);
    QCOMPARE(module.analyticsViewModel()->months()->rowCount(), 12);
    QVERIFY(!module.analyticsViewModel()->hasError());
    QVERIFY(!module.balancesViewModel()->hasError());
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
    QCOMPARE(query.value(0).toInt(), 10);
    QCOMPARE(query.value(1).toInt(), 10);
        database.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

void AccountingModuleTest::createsEditsDeletesAndRestoresTransaction()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath()).filePath(
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
        QCOMPARE(module.balancesViewModel()->activeTotalText(),
                 QStringLiteral("100.00"));
        const QString accountId = setup->accounts()->data(
            setup->accounts()->index(0, 0),
            stukit::accounting::presentation::AccountListModel::AccountIdRole)
                                      .toString();
        const QString categoryId = setup->categories()->data(
            setup->categories()->index(0, 0),
            stukit::accounting::presentation::CategoryListModel::CategoryIdRole)
                                       .toString();

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
        QCOMPARE(module.analyticsViewModel()->selectedExpenseText(),
                 QStringLiteral("25.50"));
        QCOMPARE(module.analyticsViewModel()->selectedBalanceText(),
                 QStringLiteral("-25.50"));
        QCOMPARE(module.balancesViewModel()->activeTotalText(),
                 QStringLiteral("74.50"));
        QCOMPARE(module.balancesViewModel()->accounts()->data(
                     module.balancesViewModel()->accounts()->index(0, 0),
                     stukit::accounting::presentation::
                         AccountBalanceListModel::CurrentBalanceTextRole)
                     .toString(),
                 QStringLiteral("74.50"));

        QVERIFY(setup->renameAccount(accountId, QStringLiteral("主银行卡")));
        QVERIFY(setup->renameCategory(categoryId, QStringLiteral("日常餐饮")));
        const QModelIndex refreshedTransactionIndex =
            module.overviewViewModel()->transactions()->index(0, 0);
        QCOMPARE(module.overviewViewModel()->transactions()->data(
                     refreshedTransactionIndex,
                     stukit::accounting::presentation::TransactionListModel::AccountNameRole)
                     .toString(),
                 QStringLiteral("主银行卡"));
        QCOMPARE(module.overviewViewModel()->transactions()->data(
                     refreshedTransactionIndex,
                     stukit::accounting::presentation::TransactionListModel::CategoryNameRole)
                     .toString(),
                 QStringLiteral("日常餐饮"));
        QVERIFY(setup->setCategoryArchived(categoryId, true));
        QCOMPARE(module.overviewViewModel()->transactions()->rowCount(), 1);
        QVERIFY(setup->setCategoryArchived(categoryId, false));

        const QString transactionId =
            module.overviewViewModel()->transactions()->data(
                module.overviewViewModel()->transactions()->index(0, 0),
                stukit::accounting::presentation::TransactionListModel::IdRole)
                .toString();
        QVERIFY(editor->beginEditing(transactionId));
        editor->setAmountText(QStringLiteral("30.00"));
        editor->setNote(QStringLiteral("晚饭"));
        QVERIFY2(editor->submit(), qPrintable(editor->errorMessage()));
        QCOMPARE(module.overviewViewModel()->transactions()->rowCount(), 1);
        QCOMPARE(module.overviewViewModel()->totalExpenseText(),
                 QStringLiteral("30.00"));
        QCOMPARE(module.balancesViewModel()->activeTotalText(),
                 QStringLiteral("70.00"));
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
        QCOMPARE(restored.balancesViewModel()->activeTotalText(),
                 QStringLiteral("70.00"));
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
        QCOMPARE(restored.balancesViewModel()->activeTotalText(),
                 QStringLiteral("100.00"));
    }

    {
        AccountingModule restored(appDataDirectory, settings);
        QVERIFY2(restored.initialize(QDate(2026, 10, 1)),
                 qPrintable(restored.lastError()));
        QCOMPARE(restored.overviewViewModel()->transactions()->rowCount(), 0);
        QCOMPARE(restored.balancesViewModel()->activeTotalText(),
                 QStringLiteral("100.00"));
    }
}

void AccountingModuleTest::createsAndRestoresSetupData()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("stukit-module-setup-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    const QString appDataDirectory = directory.filePath(
        QStringLiteral("app-data"));
    QString accountId;
    QString expenseCategoryId;

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
        QCOMPARE(module.balancesViewModel()->activeTotalText(),
                 QStringLiteral("123.45"));
        accountId = setup->accounts()->data(
            setup->accounts()->index(0, 0),
            stukit::accounting::presentation::AccountListModel::AccountIdRole)
                        .toString();
        expenseCategoryId = setup->categories()->data(
            setup->categories()->index(1, 0),
            stukit::accounting::presentation::CategoryListModel::CategoryIdRole)
                                .toString();
        QVERIFY(setup->renameAccount(accountId, QStringLiteral("主银行卡")));
        QVERIFY(setup->renameCategory(
            expenseCategoryId, QStringLiteral("日常餐饮")));
        QVERIFY(setup->setAccountArchived(accountId, true));
        QVERIFY(setup->setCategoryArchived(expenseCategoryId, true));
        QVERIFY(!setup->hasAccounts());
        QVERIFY(setup->hasIncomeCategories());
        QVERIFY(!setup->hasExpenseCategories());
        QCOMPARE(module.balancesViewModel()->activeTotalText(),
                 QStringLiteral("0.00"));
    }

    {
        AccountingModule restored(appDataDirectory, settings);
        QVERIFY2(restored.initialize(QDate(2026, 10, 1)),
                 qPrintable(restored.lastError()));
        auto *setup = restored.setupViewModel();
        QVERIFY(setup != nullptr);
        QCOMPARE(setup->accounts()->rowCount(), 1);
        QCOMPARE(setup->categories()->rowCount(), 2);
        QCOMPARE(setup->accounts()->data(
                     setup->accounts()->index(0, 0),
                     stukit::accounting::presentation::AccountListModel::NameRole)
                     .toString(),
                 QStringLiteral("主银行卡"));
        QCOMPARE(setup->categories()->data(
                     setup->categories()->index(1, 0),
                     stukit::accounting::presentation::CategoryListModel::NameRole)
                     .toString(),
                 QStringLiteral("日常餐饮"));
        QVERIFY(!setup->hasAccounts());
        QVERIFY(setup->hasIncomeCategories());
        QVERIFY(!setup->hasExpenseCategories());
        QCOMPARE(restored.balancesViewModel()->activeTotalText(),
                 QStringLiteral("0.00"));
        QVERIFY(setup->setAccountArchived(accountId, false));
        QVERIFY(setup->setCategoryArchived(expenseCategoryId, false));
        QVERIFY(setup->hasAccounts());
        QVERIFY(setup->hasExpenseCategories());
        QCOMPARE(restored.balancesViewModel()->activeTotalText(),
                 QStringLiteral("123.45"));
        QVERIFY(!setup->hasError());
    }
}

void AccountingModuleTest::backsUpAndRestoresDatabase()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("stukit-module-backup-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    AccountingModule module(directory.filePath(QStringLiteral("app-data")),
                            settings);
    QVERIFY2(module.initialize(QDate(2026, 10, 1)),
             qPrintable(module.lastError()));

    auto *setup = module.setupViewModel();
    QVERIFY(setup->createAccount(
        QStringLiteral("备份前账户"), QStringLiteral("12.34")));
    const QString accountId = setup->accounts()->data(
        setup->accounts()->index(0, 0),
        stukit::accounting::presentation::AccountListModel::AccountIdRole)
                                  .toString();

    const QString backupPath = directory.filePath(
        QStringLiteral("manual-backup.sqlite"));
    DataMaintenanceViewModel maintenance(module);
    QSignalSpy objectsChangedSpy(
        &maintenance,
        &DataMaintenanceViewModel::accountingObjectsChanged);
    QVERIFY2(maintenance.backupTo(QUrl::fromLocalFile(backupPath)),
             qPrintable(maintenance.errorMessage()));
    QVERIFY(!maintenance.statusMessage().isEmpty());
    QVERIFY(QFileInfo::exists(backupPath));

    QVERIFY(setup->renameAccount(accountId, QStringLiteral("备份后账户")));
    QCOMPARE(setup->accounts()->data(
                 setup->accounts()->index(0, 0),
                 stukit::accounting::presentation::AccountListModel::NameRole)
                 .toString(),
             QStringLiteral("备份后账户"));

    QPointer<stukit::accounting::presentation::AccountingOverviewViewModel>
        oldOverview = module.overviewViewModel();
    QVERIFY2(maintenance.restoreFrom(QUrl::fromLocalFile(backupPath)),
             qPrintable(maintenance.errorMessage()));
    QCOMPARE(objectsChangedSpy.count(), 1);
    QVERIFY(oldOverview.isNull());
    QCOMPARE(module.overviewViewModel()->monthLabel(),
             QStringLiteral("2026-10"));
    QCOMPARE(module.setupViewModel()->accounts()->rowCount(), 1);
    QCOMPARE(module.setupViewModel()->accounts()->data(
                 module.setupViewModel()->accounts()->index(0, 0),
                 stukit::accounting::presentation::AccountListModel::NameRole)
                 .toString(),
             QStringLiteral("备份前账户"));
    QVERIFY(QFileInfo::exists(module.lastSafetyBackupPath()));
    QVERIFY(module.lastSafetyBackupPath().contains(
        QStringLiteral("pre-restore-")));

    const QString invalidPath = directory.filePath(
        QStringLiteral("invalid.sqlite"));
    QFile invalidFile(invalidPath);
    QVERIFY(invalidFile.open(QIODevice::WriteOnly));
    QVERIFY(invalidFile.write("invalid") > 0);
    invalidFile.close();
    auto *overviewBeforeInvalidRestore = module.overviewViewModel();
    QVERIFY(!module.restoreDatabase(invalidPath));
    QCOMPARE(module.overviewViewModel(), overviewBeforeInvalidRestore);
    QCOMPARE(module.setupViewModel()->accounts()->rowCount(), 1);
}

void AccountingModuleTest::preparesDatabaseBackupBeforeUpdate()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("stukit-update-backup-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    const QString appDataDirectory = directory.filePath(
        QStringLiteral("app-data"));
    AccountingModule module(appDataDirectory, settings);
    QVERIFY2(module.initialize(QDate(2026, 10, 1)),
             qPrintable(module.lastError()));
    QVERIFY(module.setupViewModel()->createAccount(
        QStringLiteral("升级前账户"), QStringLiteral("88.00")));

    AccountingUpdatePreparation preparation(module, appDataDirectory);
    QString backupPath;
    QString errorMessage;
    QVERIFY2(preparation.prepare(
                 QStringLiteral("0.3.0"), backupPath, errorMessage),
             qPrintable(errorMessage));
    QVERIFY(QFileInfo::exists(backupPath));
    QVERIFY(backupPath.contains(QStringLiteral("before-update-to-0.3.0-")));
    const auto validation =
        stukit::accounting::SqliteBackupService::validateBackup(backupPath);
    QVERIFY2(validation.succeeded(), qPrintable(validation.detail));
}

void AccountingModuleTest::exportsTransactionsThroughMaintenanceViewModel()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("stukit-module-export-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    AccountingModule module(directory.filePath(QStringLiteral("app-data")),
                            settings);
    QVERIFY2(module.initialize(QDate(2026, 10, 1)),
             qPrintable(module.lastError()));

    auto *setup = module.setupViewModel();
    QVERIFY(setup->createAccount(
        QStringLiteral("现金钱包"), QStringLiteral("0.00")));
    QVERIFY(setup->createCategory(QStringLiteral("餐饮"), false));

    auto *editor = module.transactionEditorViewModel();
    editor->resetForNewTransaction();
    editor->setAmountText(QStringLiteral("12.34"));
    editor->setOccurredDateText(QStringLiteral("2026-10-10"));
    editor->setOccurredTimeText(QStringLiteral("12:34"));
    editor->setNote(QStringLiteral("导出测试"));
    QVERIFY2(editor->submit(), qPrintable(editor->errorMessage()));

    DataMaintenanceViewModel maintenance(module);
    const QString csvPath = directory.filePath(
        QStringLiteral("all-transactions.csv"));
    QVERIFY2(maintenance.exportTransactionsTo(QUrl::fromLocalFile(csvPath)),
             qPrintable(maintenance.errorMessage()));
    QVERIFY(maintenance.statusMessage().contains(csvPath));

    QFile csv(csvPath);
    QVERIFY(csv.open(QIODevice::ReadOnly));
    const QString contents = QString::fromUtf8(csv.readAll());
    QVERIFY(contents.contains(QStringLiteral("\"金额\"")));
    QVERIFY(contents.contains(QStringLiteral("\"12.34\"")));
    QVERIFY(contents.contains(QStringLiteral("\"现金钱包\"")));
    QVERIFY(contents.contains(QStringLiteral("\"餐饮\"")));
    QVERIFY(contents.contains(QStringLiteral("\"导出测试\"")));
}

void AccountingModuleTest::defaultsInvalidTimeZoneToUtcPlusEight()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath()).filePath(
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
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath()).filePath(
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
        QCOMPARE(module.analyticsViewModel()->timeZone().id(),
                 QByteArrayLiteral("UTC-05:00"));
        settings.sync();
    }

    UserSettings restoredSettings(settings);
    QCOMPARE(restoredSettings.accountingTimeZone().id(),
             QByteArrayLiteral("UTC-05:00"));
}

void AccountingModuleTest::persistsTaskWorkflowAcrossRestart()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("stukit-task-workflow-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    const QString appDataDirectory = directory.filePath(
        QStringLiteral("app-data"));
    QString taskId;

    {
        AccountingModule module(appDataDirectory, settings);
        QVERIFY2(module.initialize(QDate(2026, 10, 1)),
                 qPrintable(module.lastError()));
        QVERIFY(module.taskModule() != nullptr);
        auto *tasks = module.taskModule()->viewModel();
        QVERIFY(tasks != nullptr);
        QVERIFY2(tasks->save(
                     {}, QStringLiteral("完成数据结构作业"),
                     QStringLiteral("复习图与最短路径"),
                     QStringLiteral("2099-01-02 18:00"),
                     QStringLiteral("2099-01-02 17:00"),
                     2, QStringLiteral("作业"), {}),
                 qPrintable(tasks->errorMessage()));
        QCOMPARE(tasks->tasks().size(), 1);
        taskId = tasks->tasks().constFirst().toMap()
                     .value(QStringLiteral("id")).toString();
        QVERIFY(!taskId.isEmpty());
        QVERIFY(tasks->complete(taskId));
        QCOMPARE(tasks->pendingCount(), 0);
        QCOMPARE(tasks->completedCount(), 1);
        QCOMPARE(tasks->completionRateText(), QStringLiteral("100%"));
    }

    {
        AccountingModule restored(appDataDirectory, settings);
        QVERIFY2(restored.initialize(QDate(2026, 10, 1)),
                 qPrintable(restored.lastError()));
        auto *tasks = restored.taskModule()->viewModel();
        QVERIFY(tasks != nullptr);
        QCOMPARE(tasks->tasks().size(), 1);
        const QVariantMap restoredTask = tasks->tasks().constFirst().toMap();
        QCOMPARE(restoredTask.value(QStringLiteral("id")).toString(), taskId);
        QCOMPARE(restoredTask.value(QStringLiteral("title")).toString(),
                 QStringLiteral("完成数据结构作业"));
        QVERIFY(restoredTask.value(QStringLiteral("completed")).toBool());
        QVERIFY(tasks->archive(taskId));
        QCOMPARE(tasks->tasks().size(), 0);
        tasks->setIncludeArchived(true);
        QCOMPARE(tasks->tasks().size(), 1);
        QVERIFY(tasks->tasks().constFirst().toMap()
                    .value(QStringLiteral("archived")).toBool());
    }
}

void AccountingModuleTest::rejectsEmptyApplicationDataDirectory()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("stukit-settings-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    AccountingModule module(QString{}, settings);

    QVERIFY(!module.initialize(QDate(2026, 10, 1)));
    QVERIFY(module.overviewViewModel() == nullptr);
    QVERIFY(module.balancesViewModel() == nullptr);
    QVERIFY(module.analyticsViewModel() == nullptr);
    QVERIFY(module.setupViewModel() == nullptr);
    QVERIFY(module.transactionEditorViewModel() == nullptr);
    QVERIFY(!module.lastError().isEmpty());
}

QTEST_GUILESS_MAIN(AccountingModuleTest)

#include "AccountingModuleTest.moc"
