#pragma once

#include <QSqlDatabase>
#include <QString>

namespace stukit::accounting {

class SqliteDatabase final {
public:
  static constexpr int CurrentSchemaVersion = 10;

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
  [[nodiscard]] bool applyMigration2();
  [[nodiscard]] bool applyMigration3();
  [[nodiscard]] bool applyMigration4();
  [[nodiscard]] bool applyMigration5();
  [[nodiscard]] bool applyMigration6();
  [[nodiscard]] bool applyMigration7();
  [[nodiscard]] bool applyMigration8();
  [[nodiscard]] bool applyMigration9();
  [[nodiscard]] bool applyMigration10();

  QString m_connectionName;
  QSqlDatabase m_database;
  QString m_lastError;
};

} // namespace stukit::accounting
