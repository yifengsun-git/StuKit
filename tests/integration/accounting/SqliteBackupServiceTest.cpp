#include "backup/SqliteBackupService.h"
#include "database/SqliteDatabase.h"

#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

using namespace stukit::accounting;

namespace {

QString connectionName(const QString &purpose)
{
    return QStringLiteral("stukit-backup-test-%1-%2")
        .arg(purpose, QUuid::createUuid().toString(QUuid::WithoutBraces));
}

void insertAccount(QSqlDatabase database, const QString &name)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "INSERT INTO accounts(id, name, opening_balance_cents, archived) "
        "VALUES(:id, :name, 0, 0)"));
    query.bindValue(
        QStringLiteral(":id"),
        QUuid::createUuid().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":name"), name);
    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
}

void createDatabase(const QString &path, const QString &accountName)
{
    SqliteDatabase database(connectionName(QStringLiteral("create")));
    QVERIFY2(database.open(path), qPrintable(database.lastError()));
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    insertAccount(database.connection(), accountName);
}

QString firstAccountName(const QString &path)
{
    const QString name = connectionName(QStringLiteral("read"));
    const QString result = [&]() {
        QSqlDatabase database = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), name);
        database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        database.setDatabaseName(path);
        if (!database.open()) {
            return QString{};
        }
        QSqlQuery query(database);
        if (!query.exec(QStringLiteral(
                "SELECT name FROM accounts ORDER BY rowid LIMIT 1"))
            || !query.next()) {
            return QString{};
        }
        return query.value(0).toString();
    }();
    QSqlDatabase::removeDatabase(name);
    return result;
}

} // namespace

class SqliteBackupServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsConsistentBackup();
    void protectsExistingDestination();
    void rejectsInvalidAndFutureDatabases();
    void replacesClosedDatabaseAtomically();
};

void SqliteBackupServiceTest::createsConsistentBackup()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-backup-service-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString sourcePath = directory.filePath(QStringLiteral("source.sqlite"));
    const QString backupPath = directory.filePath(QStringLiteral("backup.sqlite"));

    SqliteDatabase database(connectionName(QStringLiteral("source")));
    QVERIFY2(database.open(sourcePath), qPrintable(database.lastError()));
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    insertAccount(database.connection(), QStringLiteral("银行卡"));

    SqliteBackupService service(database.connection());
    const SqliteBackupResult backup = service.createBackup(backupPath);
    QVERIFY2(backup.succeeded(), qPrintable(backup.detail));
    QCOMPARE(backup.schemaVersion, 4);
    QCOMPARE(firstAccountName(backupPath), QStringLiteral("银行卡"));

    const SqliteBackupResult validation =
        SqliteBackupService::validateBackup(backupPath);
    QVERIFY2(validation.succeeded(), qPrintable(validation.detail));
    QCOMPARE(validation.schemaVersion, 4);
}

void SqliteBackupServiceTest::protectsExistingDestination()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-backup-service-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString sourcePath = directory.filePath(QStringLiteral("source.sqlite"));
    const QString backupPath = directory.filePath(QStringLiteral("backup.sqlite"));

    SqliteDatabase database(connectionName(QStringLiteral("overwrite")));
    QVERIFY(database.open(sourcePath));
    QVERIFY(database.migrate());
    insertAccount(database.connection(), QStringLiteral("现金"));

    QFile existing(backupPath);
    QVERIFY(existing.open(QIODevice::WriteOnly));
    QVERIFY(existing.write("do not overwrite") > 0);
    existing.close();

    SqliteBackupService service(database.connection());
    QCOMPARE(service.createBackup(backupPath).error,
             SqliteBackupError::TargetExists);

    const SqliteBackupResult replaced = service.createBackup(backupPath, true);
    QVERIFY2(replaced.succeeded(), qPrintable(replaced.detail));
    QCOMPARE(firstAccountName(backupPath), QStringLiteral("现金"));
}

void SqliteBackupServiceTest::rejectsInvalidAndFutureDatabases()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-backup-service-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString invalidPath = directory.filePath(QStringLiteral("invalid.sqlite"));

    QFile invalid(invalidPath);
    QVERIFY(invalid.open(QIODevice::WriteOnly));
    QVERIFY(invalid.write("not a database") > 0);
    invalid.close();
    QCOMPARE(SqliteBackupService::validateBackup(invalidPath).error,
             SqliteBackupError::InvalidDatabase);

    const QString futurePath = directory.filePath(QStringLiteral("future.sqlite"));
    createDatabase(futurePath, QStringLiteral("未来账户"));
    const QString name = connectionName(QStringLiteral("future"));
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), name);
        database.setDatabaseName(futurePath);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral(
            "INSERT INTO schema_migrations(version, applied_at) "
            "VALUES(5, 'future')")));
    }
    QSqlDatabase::removeDatabase(name);

    const SqliteBackupResult future =
        SqliteBackupService::validateBackup(futurePath);
    QCOMPARE(future.error, SqliteBackupError::UnsupportedVersion);
    QCOMPARE(future.schemaVersion, 5);
}

void SqliteBackupServiceTest::replacesClosedDatabaseAtomically()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-backup-service-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString originalPath = directory.filePath(QStringLiteral("original.sqlite"));
    const QString backupPath = directory.filePath(QStringLiteral("backup.sqlite"));

    {
        SqliteDatabase database(connectionName(QStringLiteral("original")));
        QVERIFY(database.open(originalPath));
        QVERIFY(database.migrate());
        insertAccount(database.connection(), QStringLiteral("备份中的账户"));
        SqliteBackupService service(database.connection());
        QVERIFY(service.createBackup(backupPath).succeeded());
    }

    QFile::remove(originalPath);
    createDatabase(originalPath, QStringLiteral("当前账户"));
    QCOMPARE(firstAccountName(originalPath), QStringLiteral("当前账户"));

    const SqliteBackupResult replacement =
        SqliteBackupService::replaceDatabase(backupPath, originalPath);
    QVERIFY2(replacement.succeeded(), qPrintable(replacement.detail));
    QCOMPARE(firstAccountName(originalPath), QStringLiteral("备份中的账户"));
}

QTEST_GUILESS_MAIN(SqliteBackupServiceTest)

#include "SqliteBackupServiceTest.moc"
