#include "database/SqliteDatabase.h"

#include <QDir>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

using namespace stukit::accounting;

namespace {

QString uniqueConnectionName() {
  return QStringLiteral("stukit-test-%1")
      .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

QTemporaryDir createTemporaryDatabaseDirectory() {
  return QTemporaryDir(
      QDir::current().filePath(QStringLiteral("stukit-sqlite-test-XXXXXX")));
}

} // namespace

class SqliteDatabaseTest final : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void createsInitialSchema();
  void migrationIsIdempotent();
  void enablesForeignKeys();
};

void SqliteDatabaseTest::initTestCase() {
  QVERIFY2(QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")),
           "QSQLITE driver is not available");
}

void SqliteDatabaseTest::createsInitialSchema() {
  QTemporaryDir directory = createTemporaryDatabaseDirectory();
  QVERIFY(directory.isValid());

  SqliteDatabase database(uniqueConnectionName());
  QVERIFY2(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
           qPrintable(database.lastError()));
  QVERIFY2(database.migrate(), qPrintable(database.lastError()));

  QSet<QString> columns;
  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral("PRAGMA table_info(accounts)")),
             qPrintable(query.lastError().text()));

    while (query.next()) {
      columns.insert(query.value(1).toString());
    }
  }

  QVERIFY(columns.contains(QStringLiteral("id")));
  QVERIFY(columns.contains(QStringLiteral("name")));
  QVERIFY(columns.contains(QStringLiteral("opening_balance_cents")));
  QVERIFY(columns.contains(QStringLiteral("archived")));
}

void SqliteDatabaseTest::migrationIsIdempotent() {
  QTemporaryDir directory = createTemporaryDatabaseDirectory();
  QVERIFY(directory.isValid());

  SqliteDatabase database(uniqueConnectionName());
  QVERIFY2(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
           qPrintable(database.lastError()));
  QVERIFY2(database.migrate(), qPrintable(database.lastError()));
  QVERIFY2(database.migrate(), qPrintable(database.lastError()));

  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral(
                 "SELECT COUNT(*) FROM schema_migrations WHERE version = 1")),
             qPrintable(query.lastError().text()));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
  }
}

void SqliteDatabaseTest::enablesForeignKeys() {
  QTemporaryDir directory = createTemporaryDatabaseDirectory();
  QVERIFY(directory.isValid());

  SqliteDatabase database(uniqueConnectionName());
  QVERIFY2(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
           qPrintable(database.lastError()));

  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral("PRAGMA foreign_keys")),
             qPrintable(query.lastError().text()));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
  }
}

QTEST_GUILESS_MAIN(SqliteDatabaseTest)

#include "SqliteDatabaseTest.moc"
