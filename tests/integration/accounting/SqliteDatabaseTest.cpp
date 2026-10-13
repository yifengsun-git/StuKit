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
  void createsScheduleSchema();
  void enforcesScheduleConstraintsAndForeignKeys();
  void migrationIsIdempotent();
  void upgradesVersion1Database();
  void upgradesVersion2Database();
  void upgradesVersion3Database();
  void upgradesVersion4CourseMaterials();
  void rollsBackFailedScheduleMigration();
  void rejectsFutureSchemaVersion();
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
    QCOMPARE(query.value(0).toInt(), 7);
    QCOMPARE(query.value(1).toInt(), 7);
    QCOMPARE(query.value(2).toInt(), 7);
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

void SqliteDatabaseTest::createsScheduleSchema() {
  QTemporaryDir directory = createTemporaryDatabaseDirectory();
  QVERIFY(directory.isValid());

  SqliteDatabase database(uniqueConnectionName());
  QVERIFY2(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
           qPrintable(database.lastError()));
  QVERIFY2(database.migrate(), qPrintable(database.lastError()));

  const auto columnsFor = [&database](const QString &tableName) {
    QSet<QString> columns;
    QSqlQuery query(database.connection());
    if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(tableName))) {
      return columns;
    }
    while (query.next()) {
      columns.insert(query.value(1).toString());
    }
    return columns;
  };

  QCOMPARE(columnsFor(QStringLiteral("academic_terms")),
           QSet<QString>({QStringLiteral("id"), QStringLiteral("name"),
                          QStringLiteral("start_date"),
                          QStringLiteral("total_weeks"),
                          QStringLiteral("archived")}));
  QCOMPARE(columnsFor(QStringLiteral("courses")),
           QSet<QString>({QStringLiteral("id"), QStringLiteral("term_id"),
                          QStringLiteral("name"), QStringLiteral("teacher"),
                          QStringLiteral("location"),
                          QStringLiteral("assessment_standard"),
                          QStringLiteral("teaching_plan"),
                          QStringLiteral("color")}));
  QCOMPARE(columnsFor(QStringLiteral("course_meetings")),
           QSet<QString>({QStringLiteral("id"),
                          QStringLiteral("course_id"),
                          QStringLiteral("weekday"),
                          QStringLiteral("start_period"),
                          QStringLiteral("end_period"),
                          QStringLiteral("start_week"),
                          QStringLiteral("end_week"),
                          QStringLiteral("week_pattern")}));
  QCOMPARE(columnsFor(QStringLiteral("course_meeting_weeks")),
           QSet<QString>({QStringLiteral("meeting_id"),
                          QStringLiteral("week_number")}));
  QCOMPARE(columnsFor(QStringLiteral("schedule_period_definitions")),
           QSet<QString>({QStringLiteral("period_number"),
                          QStringLiteral("name"),
                          QStringLiteral("start_time"),
                          QStringLiteral("end_time")}));

  {
    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral(
        "SELECT COUNT(*), MIN(start_time), MAX(end_time) "
        "FROM schedule_period_definitions")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 12);
    QCOMPARE(query.value(1).toString(), QStringLiteral("08:00"));
    QCOMPARE(query.value(2).toString(), QStringLiteral("22:00"));
  }

  QSet<QString> courseIndexes;
  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral("PRAGMA index_list(courses)")),
             qPrintable(query.lastError().text()));
    while (query.next()) {
      courseIndexes.insert(query.value(1).toString());
    }
  }
  QVERIFY(courseIndexes.contains(QStringLiteral("idx_courses_term_id")));

  QSet<QString> meetingIndexes;
  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral("PRAGMA index_list(course_meetings)")),
             qPrintable(query.lastError().text()));
    while (query.next()) {
      meetingIndexes.insert(query.value(1).toString());
    }
  }
  QVERIFY(meetingIndexes.contains(
      QStringLiteral("idx_course_meetings_course_id")));
  QVERIFY(meetingIndexes.contains(
      QStringLiteral("idx_course_meetings_weekday")));

  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(QStringLiteral("PRAGMA foreign_key_list(courses)")),
             qPrintable(query.lastError().text()));
    QVERIFY(query.next());
    QCOMPARE(query.value(2).toString(), QStringLiteral("academic_terms"));
    QCOMPARE(query.value(5).toString(), QStringLiteral("CASCADE"));
    QCOMPARE(query.value(6).toString(), QStringLiteral("RESTRICT"));
  }
  {
    QSqlQuery query(database.connection());
    QVERIFY2(query.exec(
                 QStringLiteral("PRAGMA foreign_key_list(course_meetings)")),
             qPrintable(query.lastError().text()));
    QVERIFY(query.next());
    QCOMPARE(query.value(2).toString(), QStringLiteral("courses"));
    QCOMPARE(query.value(5).toString(), QStringLiteral("CASCADE"));
    QCOMPARE(query.value(6).toString(), QStringLiteral("RESTRICT"));
  }
}

void SqliteDatabaseTest::enforcesScheduleConstraintsAndForeignKeys() {
  QTemporaryDir directory = createTemporaryDatabaseDirectory();
  QVERIFY(directory.isValid());

  SqliteDatabase database(uniqueConnectionName());
  QVERIFY2(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
           qPrintable(database.lastError()));
  QVERIFY2(database.migrate(), qPrintable(database.lastError()));

  QSqlQuery query(database.connection());
  const QStringList validStatements{
      QStringLiteral("INSERT INTO academic_terms("
                     "id, name, start_date, total_weeks, archived) "
                     "VALUES('term', '秋季学期', '2026-09-07', 18, 0)"),
      QStringLiteral("INSERT INTO courses("
                     "id, term_id, name, teacher, location, color) "
                     "VALUES('course', 'term', '高等数学', '王老师', "
                     "'A101', '#3B82F6')"),
      QStringLiteral("INSERT INTO course_meetings("
                     "id, course_id, weekday, start_period, end_period, "
                     "start_week, end_week, week_pattern) "
                     "VALUES('meeting', 'course', 1, 1, 2, 1, 18, 0)"),
      QStringLiteral("INSERT INTO course_meeting_weeks("
                     "meeting_id, week_number) VALUES('meeting', 1)"),
  };
  for (const QString &statement : validStatements) {
    QVERIFY2(query.exec(statement), qPrintable(query.lastError().text()));
  }

  const QStringList invalidStatements{
      QStringLiteral("INSERT INTO academic_terms("
                     "id, name, start_date, total_weeks, archived) "
                     "VALUES('bad-date', '错误日期', 'not-a-date', 18, 0)"),
      QStringLiteral("INSERT INTO academic_terms("
                     "id, name, start_date, total_weeks, archived) "
                     "VALUES('bad-term', '错误学期', '2026-09-07', 31, 0)"),
      QStringLiteral("INSERT INTO courses("
                     "id, term_id, name, color) "
                     "VALUES('bad-color', 'term', '课程', '#3b82f6')"),
      QStringLiteral("INSERT INTO courses("
                     "id, term_id, name, color) "
                     "VALUES('orphan-course', 'missing', '课程', '#3B82F6')"),
      QStringLiteral("INSERT INTO course_meetings("
                     "id, course_id, weekday, start_period, end_period, "
                     "start_week, end_week, week_pattern) "
                     "VALUES('bad-weekday', 'course', 8, 1, 2, 1, 18, 0)"),
      QStringLiteral("INSERT INTO course_meetings("
                     "id, course_id, weekday, start_period, end_period, "
                     "start_week, end_week, week_pattern) "
                     "VALUES('bad-period', 'course', 1, 3, 2, 1, 18, 0)"),
      QStringLiteral("INSERT INTO course_meetings("
                     "id, course_id, weekday, start_period, end_period, "
                     "start_week, end_week, week_pattern) "
                     "VALUES('bad-week', 'course', 1, 1, 2, 12, 11, 0)"),
      QStringLiteral("INSERT INTO course_meetings("
                     "id, course_id, weekday, start_period, end_period, "
                     "start_week, end_week, week_pattern) "
                     "VALUES('bad-pattern', 'course', 1, 1, 2, 1, 18, 3)"),
      QStringLiteral("INSERT INTO course_meetings("
                     "id, course_id, weekday, start_period, end_period, "
                     "start_week, end_week, week_pattern) "
                     "VALUES('orphan-meeting', 'missing', 1, 1, 2, 1, 18, 0)"),
  };
  for (const QString &statement : invalidStatements) {
    QVERIFY2(!query.exec(statement), qPrintable(statement));
  }

  QVERIFY(query.exec(QStringLiteral(
      "DELETE FROM courses WHERE id = 'course'")));
  QVERIFY(query.exec(QStringLiteral(
      "SELECT COUNT(*) FROM course_meetings WHERE id = 'meeting'")));
  QVERIFY(query.next());
  QCOMPARE(query.value(0).toInt(), 0);
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
                 "WHERE version IN (2, 3, 4, 5)")),
             qPrintable(query.lastError().text()));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 4);
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
                 "WHERE version IN (3, 4, 5))")),
             qPrintable(query.lastError().text()));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
    QCOMPARE(query.value(1).toInt(), 1);
    QCOMPARE(query.value(2).toInt(), 3);
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

void SqliteDatabaseTest::upgradesVersion3Database() {
  QTemporaryDir directory = createTemporaryDatabaseDirectory();
  QVERIFY(directory.isValid());

  SqliteDatabase database(uniqueConnectionName());
  QVERIFY2(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
           qPrintable(database.lastError()));
  QVERIFY2(database.migrate(), qPrintable(database.lastError()));

  QSqlQuery query(database.connection());
  const QStringList prepareVersion3{
      QStringLiteral("DROP TABLE schedule_period_definitions"),
      QStringLiteral("DROP TABLE course_meeting_weeks"),
      QStringLiteral("DROP TABLE course_meetings"),
      QStringLiteral("DROP TABLE courses"),
      QStringLiteral("DROP TABLE academic_terms"),
      QStringLiteral("DELETE FROM schema_migrations WHERE version = 4"),
      QStringLiteral("DELETE FROM schema_migrations WHERE version = 5"),
      QStringLiteral("DELETE FROM schema_migrations WHERE version = 6"),
      QStringLiteral("DELETE FROM schema_migrations WHERE version = 7"),
      QStringLiteral("INSERT INTO accounts("
                     "id, name, opening_balance_cents, archived) "
                     "VALUES('v3-account', '版本三账户', 456, 0)"),
      QStringLiteral("INSERT INTO categories(id, name, type, archived) "
                     "VALUES('v3-category', '版本三分类', 1, 0)"),
      QStringLiteral("INSERT INTO transactions("
                     "id, type, amount_cents, account_id, category_id, "
                     "occurred_at, note) VALUES("
                     "'v3-transaction', 1, 123, 'v3-account', "
                     "'v3-category', '2026-09-08T00:00:00.000Z', '')"),
  };
  for (const QString &statement : prepareVersion3) {
    QVERIFY2(query.exec(statement), qPrintable(query.lastError().text()));
  }

  QVERIFY2(database.migrate(), qPrintable(database.lastError()));

  QVERIFY(query.exec(QStringLiteral(
      "SELECT "
      "(SELECT COUNT(*) FROM transactions WHERE id = 'v3-transaction'), "
      "(SELECT COUNT(*) FROM schema_migrations WHERE version = 4), "
      "(SELECT COUNT(*) FROM schema_migrations WHERE version = 5), "
      "(SELECT COUNT(*) FROM schema_migrations WHERE version = 6), "
      "(SELECT COUNT(*) FROM schema_migrations WHERE version = 7), "
      "(SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' "
      "AND name IN ('academic_terms', 'courses', 'course_meetings', "
      "'course_meeting_weeks', 'schedule_period_definitions'))")));
  QVERIFY(query.next());
  QCOMPARE(query.value(0).toInt(), 1);
  QCOMPARE(query.value(1).toInt(), 1);
  QCOMPARE(query.value(2).toInt(), 1);
  QCOMPARE(query.value(3).toInt(), 1);
  QCOMPARE(query.value(4).toInt(), 1);
  QCOMPARE(query.value(5).toInt(), 5);
}

void SqliteDatabaseTest::upgradesVersion4CourseMaterials() {
  QTemporaryDir directory = createTemporaryDatabaseDirectory();
  QVERIFY(directory.isValid());

  SqliteDatabase database(uniqueConnectionName());
  QVERIFY2(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
           qPrintable(database.lastError()));

  QSqlQuery query(database.connection());
  const QStringList version4Statements{
      QStringLiteral("CREATE TABLE schema_migrations ("
                     "version INTEGER PRIMARY KEY NOT NULL, "
                     "applied_at TEXT NOT NULL)"),
      QStringLiteral("CREATE TABLE academic_terms ("
                     "id TEXT PRIMARY KEY NOT NULL)"),
      QStringLiteral("CREATE TABLE courses ("
                     "id TEXT PRIMARY KEY NOT NULL, term_id TEXT NOT NULL, "
                     "name TEXT NOT NULL, teacher TEXT NOT NULL DEFAULT '', "
                     "location TEXT NOT NULL DEFAULT '', color TEXT NOT NULL)"),
      QStringLiteral("CREATE TABLE course_meetings ("
                     "id TEXT PRIMARY KEY NOT NULL, course_id TEXT NOT NULL, "
                     "weekday INTEGER NOT NULL, start_period INTEGER NOT NULL, "
                     "end_period INTEGER NOT NULL, start_week INTEGER NOT NULL, "
                     "end_week INTEGER NOT NULL, week_pattern INTEGER NOT NULL)"),
      QStringLiteral("INSERT INTO schema_migrations(version, applied_at) "
                     "VALUES(4, 'version-four')"),
      QStringLiteral("INSERT INTO academic_terms(id) VALUES('term')"),
      QStringLiteral("INSERT INTO courses("
                     "id, term_id, name, teacher, location, color) VALUES("
                     "'course', 'term', '高等数学', '王老师', 'A101', "
                     "'#3B82F6')"),
  };
  for (const QString &statement : version4Statements) {
    QVERIFY2(query.exec(statement), qPrintable(query.lastError().text()));
  }

  QVERIFY2(database.migrate(), qPrintable(database.lastError()));
  QVERIFY(query.exec(QStringLiteral(
      "SELECT assessment_standard, teaching_plan FROM courses "
      "WHERE id = 'course'")));
  QVERIFY(query.next());
  QCOMPARE(query.value(0).toString(), QString{});
  QCOMPARE(query.value(1).toString(), QString{});

  QVERIFY(query.exec(QStringLiteral(
      "SELECT COUNT(*) FROM schema_migrations WHERE version = 5")));
  QVERIFY(query.next());
  QCOMPARE(query.value(0).toInt(), 1);

  QVERIFY(query.exec(QStringLiteral(
      "SELECT COUNT(*) FROM schema_migrations WHERE version = 6")));
  QVERIFY(query.next());
  QCOMPARE(query.value(0).toInt(), 1);

  QVERIFY(query.exec(QStringLiteral(
      "SELECT COUNT(*) FROM schema_migrations WHERE version = 7")));
  QVERIFY(query.next());
  QCOMPARE(query.value(0).toInt(), 1);
}

void SqliteDatabaseTest::rollsBackFailedScheduleMigration() {
  QTemporaryDir directory = createTemporaryDatabaseDirectory();
  QVERIFY(directory.isValid());

  SqliteDatabase database(uniqueConnectionName());
  QVERIFY2(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
           qPrintable(database.lastError()));
  QVERIFY2(database.migrate(), qPrintable(database.lastError()));

  QSqlQuery query(database.connection());
  const QStringList prepareBrokenVersion3{
      QStringLiteral("DROP TABLE schedule_period_definitions"),
      QStringLiteral("DROP TABLE course_meeting_weeks"),
      QStringLiteral("DROP TABLE course_meetings"),
      QStringLiteral("DROP TABLE courses"),
      QStringLiteral("DROP TABLE academic_terms"),
      QStringLiteral("DELETE FROM schema_migrations WHERE version = 4"),
      QStringLiteral("DELETE FROM schema_migrations WHERE version = 5"),
      QStringLiteral("DELETE FROM schema_migrations WHERE version = 6"),
      QStringLiteral("DELETE FROM schema_migrations WHERE version = 7"),
      QStringLiteral("CREATE TABLE courses(dummy TEXT)"),
  };
  for (const QString &statement : prepareBrokenVersion3) {
    QVERIFY2(query.exec(statement), qPrintable(query.lastError().text()));
  }

  QVERIFY(!database.migrate());
  QVERIFY(!database.lastError().isEmpty());

  QVERIFY(query.exec(QStringLiteral(
      "SELECT "
      "(SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' "
      "AND name = 'academic_terms'), "
      "(SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' "
      "AND name = 'courses'), "
      "(SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' "
      "AND name = 'course_meetings'), "
      "(SELECT COUNT(*) FROM schema_migrations WHERE version = 4)")));
  QVERIFY(query.next());
  QCOMPARE(query.value(0).toInt(), 0);
  QCOMPARE(query.value(1).toInt(), 1);
  QCOMPARE(query.value(2).toInt(), 0);
  QCOMPARE(query.value(3).toInt(), 0);
}

void SqliteDatabaseTest::rejectsFutureSchemaVersion() {
  QTemporaryDir directory = createTemporaryDatabaseDirectory();
  QVERIFY(directory.isValid());

  SqliteDatabase database(uniqueConnectionName());
  QVERIFY2(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
           qPrintable(database.lastError()));
  QVERIFY2(database.migrate(), qPrintable(database.lastError()));

  QSqlQuery query(database.connection());
  QVERIFY2(query.exec(QStringLiteral(
               "INSERT INTO schema_migrations(version, applied_at) "
               "VALUES(8, 'future-test')")),
           qPrintable(query.lastError().text()));

  QVERIFY(!database.migrate());
  QVERIFY(database.lastError().contains(QStringLiteral("newer")));
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
