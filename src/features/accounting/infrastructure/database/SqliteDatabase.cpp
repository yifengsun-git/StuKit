#include "database/SqliteDatabase.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>

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

  if (version < 1 && !applyMigration1()) {
    return false;
  }

  if (version < 2 && !applyMigration2()) {
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

} // namespace stukit::accounting
