#include "AccountingModule.h"
#include "AccountingOverviewViewModel.h"
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
    QVERIFY(QFileInfo::exists(module.databasePath()));
    QVERIFY(module.databasePath().startsWith(directory.path()));
    QCOMPARE(module.overviewViewModel()->monthLabel(),
             QStringLiteral("2026-10"));
    QVERIFY(module.overviewViewModel()->isEmpty());
    QVERIFY(!module.overviewViewModel()->hasError());

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
    QVERIFY(!module.lastError().isEmpty());
}

QTEST_GUILESS_MAIN(AccountingModuleTest)

#include "AccountingModuleTest.moc"
