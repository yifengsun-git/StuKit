#pragma once

#include <QSqlDatabase>
#include <QString>

namespace stukit::accounting {

class SqliteDatabase final {
public:
  explicit SqliteDatabase(QString connectionName);
  ~SqliteDatabase();

  SqliteDatabase(const SqliteDatabase &) = delete;
  SqliteDatabase &operator=(const SqliteDatabase &) = delete;
  SqliteDatabase(SqliteDatabase &&) = delete;
  SqliteDatabase &operator=(SqliteDatabase &&) = delete;

  [[nodiscard]] bool open(const QString &filePath);
  [[nodiscard]] bool migrate();

  [[nodiscard]] QSqlDatabase connection() const;
  [[nodiscard]] const QString &lastError() const noexcept;

private:
  [[nodiscard]] bool ensureMigrationTable();
  [[nodiscard]] bool readCurrentVersion(int &version);
  [[nodiscard]] bool applyMigration1();

  QString m_connectionName;
  QSqlDatabase m_database;
  QString m_lastError;
};

} // namespace stukit::accounting
