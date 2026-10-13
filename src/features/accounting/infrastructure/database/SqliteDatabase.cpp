#include "database/SqliteDatabase.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>

#include <utility>

namespace stukit::accounting {

SqliteDatabase::SqliteDatabase(QString connectionName)
    : m_connectionName(std::move(connectionName)),
      m_database(QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                           m_connectionName)) {}

SqliteDatabase::~SqliteDatabase() {
  if (m_database.isValid()) {
    m_database.close();
  }

  m_database = QSqlDatabase{};
  QSqlDatabase::removeDatabase(m_connectionName);
}

bool SqliteDatabase::open(const QString &filePath) {
  m_lastError.clear();
  m_database.setDatabaseName(filePath);

  if (!m_database.open()) {
    m_lastError = m_database.lastError().text();
    return false;
  }

  QSqlQuery foreignKeys(m_database);
  if (!foreignKeys.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
    m_lastError = foreignKeys.lastError().text();
    return false;
  }

  return true;
}

bool SqliteDatabase::migrate() {
  m_lastError.clear();

  if (!m_database.isOpen()) {
    m_lastError = QStringLiteral("Database is not open");
    return false;
  }

  if (!ensureMigrationTable()) {
    return false;
  }

  int version = 0;
  if (!readCurrentVersion(version)) {
    return false;
  }

  if (version > CurrentSchemaVersion) {
    m_lastError = QStringLiteral("Database schema is newer than this app");
    return false;
  }

  if (version < 1 && !applyMigration1()) {
    return false;
  }

  if (version < 2 && !applyMigration2()) {
    return false;
  }

  if (version < 3 && !applyMigration3()) {
    return false;
  }

  if (version < 4 && !applyMigration4()) {
    return false;
  }

  if (version < 5 && !applyMigration5()) {
    return false;
  }

  if (version < 6 && !applyMigration6()) {
    return false;
  }

  if (version < 7 && !applyMigration7()) {
    return false;
  }

  return true;
}

QSqlDatabase SqliteDatabase::connection() const { return m_database; }

const QString &SqliteDatabase::lastError() const noexcept {
  return m_lastError;
}

bool SqliteDatabase::ensureMigrationTable() {
  QSqlQuery query(m_database);
  if (!query.exec(
          QStringLiteral("CREATE TABLE IF NOT EXISTS schema_migrations ("
                         "version INTEGER PRIMARY KEY NOT NULL, "
                         "applied_at TEXT NOT NULL)"))) {
    m_lastError = query.lastError().text();
    return false;
  }

  return true;
}

bool SqliteDatabase::readCurrentVersion(int &version) {
  QSqlQuery query(m_database);
  if (!query.exec(QStringLiteral(
          "SELECT COALESCE(MAX(version), 0) FROM schema_migrations"))) {
    m_lastError = query.lastError().text();
    return false;
  }

  if (!query.next()) {
    m_lastError = QStringLiteral("Could not read schema version");
    return false;
  }

  version = query.value(0).toInt();
  return true;
}

bool SqliteDatabase::applyMigration1() {
  if (!m_database.transaction()) {
    m_lastError = m_database.lastError().text();
    return false;
  }

  QSqlQuery createAccounts(m_database);
  if (!createAccounts.exec(
          QStringLiteral("CREATE TABLE accounts ("
                         "id TEXT PRIMARY KEY NOT NULL, "
                         "name TEXT NOT NULL, "
                         "opening_balance_cents INTEGER NOT NULL, "
                         "archived INTEGER NOT NULL DEFAULT 0 "
                         "CHECK (archived IN (0, 1)))"))) {
    m_lastError = createAccounts.lastError().text();
    m_database.rollback();
    return false;
  }

  QSqlQuery recordMigration(m_database);
  recordMigration.prepare(
      QStringLiteral("INSERT INTO schema_migrations(version, applied_at) "
                     "VALUES(:version, :appliedAt)"));
  recordMigration.bindValue(QStringLiteral(":version"), 1);
  recordMigration.bindValue(
      QStringLiteral(":appliedAt"),
      QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));

  if (!recordMigration.exec()) {
    m_lastError = recordMigration.lastError().text();
    m_database.rollback();
    return false;
  }

  if (!m_database.commit()) {
    m_lastError = m_database.lastError().text();
    m_database.rollback();
    return false;
  }

  return true;
}

bool SqliteDatabase::applyMigration2() {
  if (!m_database.transaction()) {
    m_lastError = m_database.lastError().text();
    return false;
  }

  QSqlQuery createCategories(m_database);
  if (!createCategories.exec(
          QStringLiteral("CREATE TABLE categories ("
                         "id TEXT PRIMARY KEY NOT NULL, "
                         "name TEXT NOT NULL, "
                         "type INTEGER NOT NULL "
                         "CHECK (type IN (0, 1)), "
                         "archived INTEGER NOT NULL DEFAULT 0 "
                         "CHECK (archived IN (0, 1)))"))) {
    m_lastError = createCategories.lastError().text();
    m_database.rollback();
    return false;
  }

  QSqlQuery recordMigration(m_database);
  recordMigration.prepare(
      QStringLiteral("INSERT INTO schema_migrations(version, applied_at) "
                     "VALUES(:version, :appliedAt)"));
  recordMigration.bindValue(QStringLiteral(":version"), 2);
  recordMigration.bindValue(
      QStringLiteral(":appliedAt"),
      QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));

  if (!recordMigration.exec()) {
    m_lastError = recordMigration.lastError().text();
    m_database.rollback();
    return false;
  }

  if (!m_database.commit()) {
    m_lastError = m_database.lastError().text();
    m_database.rollback();
    return false;
  }

  return true;
}

bool SqliteDatabase::applyMigration3() {
  if (!m_database.transaction()) {
    m_lastError = m_database.lastError().text();
    return false;
  }

  QSqlQuery query(m_database);
  const QStringList statements{
      QStringLiteral("CREATE TABLE transactions ("
                     "id TEXT PRIMARY KEY NOT NULL, "
                     "type INTEGER NOT NULL CHECK (type IN (0, 1)), "
                     "amount_cents INTEGER NOT NULL CHECK (amount_cents > 0), "
                     "account_id TEXT NOT NULL, "
                     "category_id TEXT NOT NULL, "
                     "occurred_at TEXT NOT NULL, "
                     "note TEXT NOT NULL DEFAULT '', "
                     "FOREIGN KEY (account_id) REFERENCES accounts(id) "
                     "ON UPDATE CASCADE ON DELETE RESTRICT, "
                     "FOREIGN KEY (category_id) REFERENCES categories(id) "
                     "ON UPDATE CASCADE ON DELETE RESTRICT)"),
      QStringLiteral("CREATE INDEX idx_transactions_occurred_at "
                     "ON transactions(occurred_at)"),
      QStringLiteral("CREATE INDEX idx_transactions_account_id "
                     "ON transactions(account_id)"),
      QStringLiteral("CREATE INDEX idx_transactions_category_id "
                     "ON transactions(category_id)"),
  };

  for (const QString &statement : statements) {
    if (!query.exec(statement)) {
      m_lastError = query.lastError().text();
      m_database.rollback();
      return false;
    }
  }

  query.prepare(
      QStringLiteral("INSERT INTO schema_migrations(version, applied_at) "
                     "VALUES(:version, :appliedAt)"));
  query.bindValue(QStringLiteral(":version"), 3);
  query.bindValue(QStringLiteral(":appliedAt"),
                  QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));

  if (!query.exec()) {
    m_lastError = query.lastError().text();
    m_database.rollback();
    return false;
  }

  if (!m_database.commit()) {
    m_lastError = m_database.lastError().text();
    m_database.rollback();
    return false;
  }

  return true;
}

bool SqliteDatabase::applyMigration4() {
  if (!m_database.transaction()) {
    m_lastError = m_database.lastError().text();
    return false;
  }

  QSqlQuery query(m_database);
  const QStringList statements{
      QStringLiteral("CREATE TABLE academic_terms ("
                     "id TEXT PRIMARY KEY NOT NULL, "
                     "name TEXT NOT NULL "
                     "CHECK (length(trim(name)) BETWEEN 1 AND 40), "
                     "start_date TEXT NOT NULL "
                     "CHECK (date(start_date) IS NOT NULL "
                     "AND start_date = date(start_date)), "
                     "total_weeks INTEGER NOT NULL "
                     "CHECK (total_weeks BETWEEN 1 AND 30), "
                     "archived INTEGER NOT NULL DEFAULT 0 "
                     "CHECK (archived IN (0, 1)))"),
      QStringLiteral("CREATE TABLE courses ("
                     "id TEXT PRIMARY KEY NOT NULL, "
                     "term_id TEXT NOT NULL, "
                     "name TEXT NOT NULL "
                     "CHECK (length(trim(name)) BETWEEN 1 AND 80), "
                     "teacher TEXT NOT NULL DEFAULT '' "
                     "CHECK (length(teacher) <= 60), "
                     "location TEXT NOT NULL DEFAULT '' "
                     "CHECK (length(location) <= 80), "
                     "color TEXT NOT NULL "
                     "CHECK (length(color) = 7 "
                     "AND substr(color, 1, 1) = '#' "
                     "AND substr(color, 2, 6) "
                     "NOT GLOB '*[^0-9A-F]*'), "
                     "FOREIGN KEY (term_id) REFERENCES academic_terms(id) "
                     "ON UPDATE CASCADE ON DELETE RESTRICT)"),
      QStringLiteral("CREATE TABLE course_meetings ("
                     "id TEXT PRIMARY KEY NOT NULL, "
                     "course_id TEXT NOT NULL, "
                     "weekday INTEGER NOT NULL "
                     "CHECK (weekday BETWEEN 1 AND 7), "
                     "start_period INTEGER NOT NULL "
                     "CHECK (start_period BETWEEN 1 AND 20), "
                     "end_period INTEGER NOT NULL "
                     "CHECK (end_period BETWEEN 1 AND 20 "
                     "AND start_period <= end_period), "
                     "start_week INTEGER NOT NULL "
                     "CHECK (start_week BETWEEN 1 AND 30), "
                     "end_week INTEGER NOT NULL "
                     "CHECK (end_week BETWEEN 1 AND 30 "
                     "AND start_week <= end_week), "
                     "week_pattern INTEGER NOT NULL "
                     "CHECK (week_pattern IN (0, 1, 2)), "
                     "FOREIGN KEY (course_id) REFERENCES courses(id) "
                     "ON UPDATE CASCADE ON DELETE RESTRICT)"),
      QStringLiteral("CREATE INDEX idx_courses_term_id "
                     "ON courses(term_id)"),
      QStringLiteral("CREATE INDEX idx_course_meetings_course_id "
                     "ON course_meetings(course_id)"),
      QStringLiteral("CREATE INDEX idx_course_meetings_weekday "
                     "ON course_meetings(weekday)"),
  };

  for (const QString &statement : statements) {
    if (!query.exec(statement)) {
      m_lastError = query.lastError().text();
      m_database.rollback();
      return false;
    }
  }

  query.prepare(
      QStringLiteral("INSERT INTO schema_migrations(version, applied_at) "
                     "VALUES(:version, :appliedAt)"));
  query.bindValue(QStringLiteral(":version"), 4);
  query.bindValue(QStringLiteral(":appliedAt"),
                  QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));

  if (!query.exec()) {
    m_lastError = query.lastError().text();
    m_database.rollback();
    return false;
  }

  if (!m_database.commit()) {
    m_lastError = m_database.lastError().text();
    m_database.rollback();
    return false;
  }

  return true;
}

bool SqliteDatabase::applyMigration5() {
  if (!m_database.transaction()) {
    m_lastError = m_database.lastError().text();
    return false;
  }

  QSqlQuery query(m_database);
  const QStringList statements{
      QStringLiteral("ALTER TABLE courses ADD COLUMN assessment_standard "
                     "TEXT NOT NULL DEFAULT '' "
                     "CHECK (length(assessment_standard) <= 2000)"),
      QStringLiteral("ALTER TABLE courses ADD COLUMN teaching_plan "
                     "TEXT NOT NULL DEFAULT '' "
                     "CHECK (length(teaching_plan) <= 5000)"),
  };

  for (const QString &statement : statements) {
    if (!query.exec(statement)) {
      m_lastError = query.lastError().text();
      m_database.rollback();
      return false;
    }
  }

  query.prepare(
      QStringLiteral("INSERT INTO schema_migrations(version, applied_at) "
                     "VALUES(:version, :appliedAt)"));
  query.bindValue(QStringLiteral(":version"), 5);
  query.bindValue(QStringLiteral(":appliedAt"),
                  QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));

  if (!query.exec()) {
    m_lastError = query.lastError().text();
    m_database.rollback();
    return false;
  }

  if (!m_database.commit()) {
    m_lastError = m_database.lastError().text();
    m_database.rollback();
    return false;
  }

  return true;
}

bool SqliteDatabase::applyMigration6() {
  if (!m_database.transaction()) {
    m_lastError = m_database.lastError().text();
    return false;
  }

  QSqlQuery query(m_database);
  const QStringList statements{
      QStringLiteral("CREATE TABLE course_meeting_weeks ("
                     "meeting_id TEXT NOT NULL, "
                     "week_number INTEGER NOT NULL "
                     "CHECK (week_number BETWEEN 1 AND 30), "
                     "PRIMARY KEY (meeting_id, week_number), "
                     "FOREIGN KEY (meeting_id) REFERENCES course_meetings(id) "
                     "ON UPDATE CASCADE ON DELETE CASCADE)"),
      QStringLiteral("CREATE INDEX idx_course_meeting_weeks_week_number "
                     "ON course_meeting_weeks(week_number)"),
      QStringLiteral(
          "WITH RECURSIVE expanded(meeting_id, week_number, end_week, "
          "week_pattern) AS ("
          "SELECT id, start_week, end_week, week_pattern "
          "FROM course_meetings "
          "UNION ALL "
          "SELECT meeting_id, week_number + 1, end_week, week_pattern "
          "FROM expanded WHERE week_number < end_week) "
          "INSERT INTO course_meeting_weeks(meeting_id, week_number) "
          "SELECT meeting_id, week_number FROM expanded "
          "WHERE week_pattern = 0 "
          "OR (week_pattern = 1 AND week_number % 2 = 1) "
          "OR (week_pattern = 2 AND week_number % 2 = 0)"),
      QStringLiteral("CREATE TRIGGER delete_course_meetings "
                     "BEFORE DELETE ON courses "
                     "BEGIN DELETE FROM course_meetings "
                     "WHERE course_id = OLD.id; END"),
  };

  for (const QString &statement : statements) {
    if (!query.exec(statement)) {
      m_lastError = query.lastError().text();
      m_database.rollback();
      return false;
    }
  }

  query.prepare(
      QStringLiteral("INSERT INTO schema_migrations(version, applied_at) "
                     "VALUES(:version, :appliedAt)"));
  query.bindValue(QStringLiteral(":version"), 6);
  query.bindValue(QStringLiteral(":appliedAt"),
                  QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));

  if (!query.exec()) {
    m_lastError = query.lastError().text();
    m_database.rollback();
    return false;
  }

  if (!m_database.commit()) {
    m_lastError = m_database.lastError().text();
    m_database.rollback();
    return false;
  }

  return true;
}

bool SqliteDatabase::applyMigration7() {
  if (!m_database.transaction()) {
    m_lastError = m_database.lastError().text();
    return false;
  }

  QSqlQuery query(m_database);
  const QStringList statements{
      QStringLiteral(
          "CREATE TABLE schedule_period_definitions ("
          "period_number INTEGER PRIMARY KEY NOT NULL "
          "CHECK (period_number BETWEEN 1 AND 20), "
          "name TEXT NOT NULL "
          "CHECK (length(trim(name)) BETWEEN 1 AND 20), "
          "start_time TEXT NOT NULL "
          "CHECK (length(start_time) = 5 "
          "AND substr(start_time, 3, 1) = ':' "
          "AND time(start_time) IS NOT NULL), "
          "end_time TEXT NOT NULL "
          "CHECK (length(end_time) = 5 "
          "AND substr(end_time, 3, 1) = ':' "
          "AND time(end_time) IS NOT NULL "
          "AND start_time < end_time))"),
      QStringLiteral(
          "INSERT INTO schedule_period_definitions("
          "period_number, name, start_time, end_time) VALUES "
          "(1, '第 1 节', '08:00', '08:45'), "
          "(2, '第 2 节', '08:55', '09:40'), "
          "(3, '第 3 节', '10:00', '10:45'), "
          "(4, '第 4 节', '10:55', '11:40'), "
          "(5, '第 5 节', '13:30', '14:15'), "
          "(6, '第 6 节', '14:25', '15:10'), "
          "(7, '第 7 节', '15:30', '16:15'), "
          "(8, '第 8 节', '16:25', '17:10'), "
          "(9, '第 9 节', '18:30', '19:15'), "
          "(10, '第 10 节', '19:25', '20:10'), "
          "(11, '第 11 节', '20:20', '21:05'), "
          "(12, '第 12 节', '21:15', '22:00')"),
  };

  for (const QString &statement : statements) {
    if (!query.exec(statement)) {
      m_lastError = query.lastError().text();
      m_database.rollback();
      return false;
    }
  }

  query.prepare(
      QStringLiteral("INSERT INTO schema_migrations(version, applied_at) "
                     "VALUES(:version, :appliedAt)"));
  query.bindValue(QStringLiteral(":version"), CurrentSchemaVersion);
  query.bindValue(QStringLiteral(":appliedAt"),
                  QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));

  if (!query.exec()) {
    m_lastError = query.lastError().text();
    m_database.rollback();
    return false;
  }

  if (!m_database.commit()) {
    m_lastError = m_database.lastError().text();
    m_database.rollback();
    return false;
  }

  return true;
}

} // namespace stukit::accounting
