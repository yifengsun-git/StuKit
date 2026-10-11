#pragma once

#include <QSqlDatabase>
#include <QString>

namespace stukit::accounting {

enum class SqliteBackupError
{
    None,
    SourceUnavailable,
    InvalidPath,
    SameFile,
    TargetExists,
    IoFailure,
    QueryFailure,
    InvalidDatabase,
    UnsupportedVersion
};

struct SqliteBackupResult
{
    SqliteBackupError error{SqliteBackupError::None};
    QString detail;
    int schemaVersion{0};

    [[nodiscard]] bool succeeded() const noexcept
    {
        return error == SqliteBackupError::None;
    }
};

class SqliteBackupService final
{
public:
    explicit SqliteBackupService(QSqlDatabase database);

    [[nodiscard]] SqliteBackupResult createBackup(
        const QString &destinationPath,
        bool replaceExisting = false) const;

    [[nodiscard]] static SqliteBackupResult validateBackup(
        const QString &filePath);
    [[nodiscard]] static SqliteBackupResult replaceDatabase(
        const QString &backupPath,
        const QString &databasePath);

private:
    [[nodiscard]] static bool pathsReferToSameFile(
        const QString &left,
        const QString &right);
    [[nodiscard]] static SqliteBackupResult copyAtomically(
        const QString &sourcePath,
        const QString &destinationPath);

    QSqlDatabase m_database;
};

} // namespace stukit::accounting
