#include "database/SqliteDatabase.h"

#include <QDir>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
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
  void createsCategorySchema();
  void createsTransactionSchema();
  void migrationIsIdempotent();
  void upgradesVersion1Database();
  void upgradesVersion2Database();
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
                 "SELECT COUNT(*), COUNT(DISTINCT version), MAX(version) "
                 "FROM schema_migrations")),
             qPrintable(query.lastError().text()));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 3);
    QCOMPARE(query.value(1).toInt(), 3);
    QCOMPARE(query.value(2).toInt(), 3);
  }
}

void SqliteDatabaseTest::createsCategorySchema() {
  QTemporaryDir directory = createTemporaryDatabaseDirectory();
  QVERIFY(directory.isValid());

  SqliteDatabase database(uniqueConnectionName());
  QVERIFY2(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
           qPrintable(database.lastError()));
  QVERIFY2(database.migrate(), qPrintable(database.lastError()));

  QSet<QString> columns;
  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral("PRAGMA table_info(categories)")),
             qPrintable(query.lastError().text()));

    while (query.next()) {
      columns.insert(query.value(1).toString());
    }
  }

  QVERIFY(columns.contains(QStringLiteral("id")));
  QVERIFY(columns.contains(QStringLiteral("name")));
  QVERIFY(columns.contains(QStringLiteral("type")));
  QVERIFY(columns.contains(QStringLiteral("archived")));
}

void SqliteDatabaseTest::createsTransactionSchema() {
  QTemporaryDir directory = createTemporaryDatabaseDirectory();
  QVERIFY(directory.isValid());

  SqliteDatabase database(uniqueConnectionName());
  QVERIFY2(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
           qPrintable(database.lastError()));
  QVERIFY2(database.migrate(), qPrintable(database.lastError()));

  QSet<QString> columns;
  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral("PRAGMA table_info(transactions)")),
             qPrintable(query.lastError().text()));
    while (query.next()) {
      columns.insert(query.value(1).toString());
    }
  }

  const QSet<QString> expectedColumns{
      QStringLiteral("id"),          QStringLiteral("type"),
      QStringLiteral("amount_cents"), QStringLiteral("account_id"),
      QStringLiteral("category_id"), QStringLiteral("occurred_at"),
      QStringLiteral("note"),
  };
  QCOMPARE(columns, expectedColumns);

  QSet<QString> indexes;
  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral("PRAGMA index_list(transactions)")),
             qPrintable(query.lastError().text()));
    while (query.next()) {
      indexes.insert(query.value(1).toString());
    }
  }

  QVERIFY(indexes.contains(QStringLiteral("idx_transactions_occurred_at")));
  QVERIFY(indexes.contains(QStringLiteral("idx_transactions_account_id")));
  QVERIFY(indexes.contains(QStringLiteral("idx_transactions_category_id")));
}

void SqliteDatabaseTest::upgradesVersion1Database() {
  QTemporaryDir directory = createTemporaryDatabaseDirectory();
  QVERIFY(directory.isValid());

  SqliteDatabase database(uniqueConnectionName());
  QVERIFY2(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
           qPrintable(database.lastError()));

  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral(
                 "CREATE TABLE schema_migrations ("
                 "version INTEGER PRIMARY KEY NOT NULL, "
                 "applied_at TEXT NOT NULL)")),
             qPrintable(query.lastError().text()));
    QVERIFY2(query.exec(QStringLiteral(
                 "CREATE TABLE accounts ("
                 "id TEXT PRIMARY KEY NOT NULL, "
                 "name TEXT NOT NULL, "
                 "opening_balance_cents INTEGER NOT NULL, "
                 "archived INTEGER NOT NULL DEFAULT 0 "
                 "CHECK (archived IN (0, 1)))")),
             qPrintable(query.lastError().text()));
    QVERIFY2(query.exec(QStringLiteral(
                 "INSERT INTO schema_migrations(version, applied_at) "
                 "VALUES(1, 'legacy-test')")),
             qPrintable(query.lastError().text()));
    QVERIFY2(query.exec(QStringLiteral(
                 "INSERT INTO accounts("
                 "id, name, opening_balance_cents, archived) "
                 "VALUES('legacy-id', '旧账户', 123, 0)")),
             qPrintable(query.lastError().text()));
  }

  QVERIFY2(database.migrate(), qPrintable(database.lastError()));

  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral(
                 "SELECT name, opening_balance_cents FROM accounts "
                 "WHERE id = 'legacy-id'")),
             qPrintable(query.lastError().text()));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("旧账户"));
    QCOMPARE(query.value(1).toLongLong(), qint64{123});
  }

  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral(
                 "SELECT COUNT(*) FROM schema_migrations "
                 "WHERE version IN (2, 3)")),
             qPrintable(query.lastError().text()));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 2);
  }

  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral(
                 "SELECT COUNT(*) FROM sqlite_master "
                 "WHERE type = 'table' AND name = 'categories'")),
             qPrintable(query.lastError().text()));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
  }
}

void SqliteDatabaseTest::upgradesVersion2Database() {
  QTemporaryDir directory = createTemporaryDatabaseDirectory();
  QVERIFY(directory.isValid());

  SqliteDatabase database(uniqueConnectionName());
  QVERIFY2(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
           qPrintable(database.lastError()));

  {
    QSqlQuery query(database.connection());
    const QStringList statements{
        QStringLiteral("CREATE TABLE schema_migrations ("
                       "version INTEGER PRIMARY KEY NOT NULL, "
                       "applied_at TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE accounts ("
                       "id TEXT PRIMARY KEY NOT NULL, name TEXT NOT NULL, "
                       "opening_balance_cents INTEGER NOT NULL, "
                       "archived INTEGER NOT NULL DEFAULT 0 "
                       "CHECK (archived IN (0, 1)))"),
        QStringLiteral("CREATE TABLE categories ("
                       "id TEXT PRIMARY KEY NOT NULL, name TEXT NOT NULL, "
                       "type INTEGER NOT NULL CHECK (type IN (0, 1)), "
                       "archived INTEGER NOT NULL DEFAULT 0 "
                       "CHECK (archived IN (0, 1)))"),
        QStringLiteral("INSERT INTO schema_migrations(version, applied_at) "
                       "VALUES(1, 'legacy-test')"),
        QStringLiteral("INSERT INTO schema_migrations(version, applied_at) "
                       "VALUES(2, 'legacy-test')"),
        QStringLiteral("INSERT INTO accounts("
                       "id, name, opening_balance_cents, archived) "
                       "VALUES('legacy-account', '旧账户', 123, 0)"),
        QStringLiteral("INSERT INTO categories(id, name, type, archived) "
                       "VALUES('legacy-category', '旧分类', 1, 0)"),
    };

    for (const QString &statement : statements) {
      QVERIFY2(query.exec(statement), qPrintable(query.lastError().text()));
    }
  }

  QVERIFY2(database.migrate(), qPrintable(database.lastError()));

  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral(
                 "SELECT (SELECT COUNT(*) FROM accounts "
                 "WHERE id = 'legacy-account'), "
                 "(SELECT COUNT(*) FROM categories "
                 "WHERE id = 'legacy-category'), "
                 "(SELECT COUNT(*) FROM schema_migrations "
                 "WHERE version = 3)")),
             qPrintable(query.lastError().text()));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
    QCOMPARE(query.value(1).toInt(), 1);
    QCOMPARE(query.value(2).toInt(), 1);
  }

  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral(
                 "SELECT COUNT(*) FROM sqlite_master "
                 "WHERE type = 'table' AND name = 'transactions'")),
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
