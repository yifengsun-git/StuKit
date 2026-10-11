#include "backup/SqliteBackupService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryFile>
#include <QUuid>

#include <utility>

namespace stukit::accounting {

namespace {

constexpr int currentSchemaVersion = 3;

SqliteBackupResult failure(SqliteBackupError error, QString detail)
{
    return SqliteBackupResult{
        .error = error,
        .detail = std::move(detail),
    };
}

bool tableExists(QSqlDatabase &database, const QString &tableName)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM sqlite_master "
        "WHERE type = 'table' AND name = :name"));
    query.bindValue(QStringLiteral(":name"), tableName);
    return query.exec() && query.next() && query.value(0).toInt() == 1;
}

} // namespace

SqliteBackupService::SqliteBackupService(QSqlDatabase database)
    : m_database(std::move(database))
{
}

SqliteBackupResult SqliteBackupService::createBackup(
    const QString &destinationPath,
    bool replaceExisting) const
{
    if (!m_database.isValid() || !m_database.isOpen()) {
        return failure(
            SqliteBackupError::SourceUnavailable,
            QStringLiteral("Database is not open"));
    }

    const QString normalizedDestination = destinationPath.trimmed();
    const QFileInfo destinationInfo(normalizedDestination);
    if (normalizedDestination.isEmpty()
        || destinationInfo.fileName().isEmpty()) {
        return failure(
            SqliteBackupError::InvalidPath,
            QStringLiteral("Backup destination is empty"));
    }
    if (pathsReferToSameFile(
            m_database.databaseName(), normalizedDestination)) {
        return failure(
            SqliteBackupError::SameFile,
            QStringLiteral("Backup destination is the active database"));
    }
    if (destinationInfo.exists() && !replaceExisting) {
        return failure(
            SqliteBackupError::TargetExists,
            QStringLiteral("Backup destination already exists"));
    }

    const QString destinationDirectory = destinationInfo.absolutePath();
    if (!QFileInfo(destinationDirectory).isDir()) {
        return failure(
            SqliteBackupError::InvalidPath,
            QStringLiteral("Backup destination directory does not exist"));
    }

    QTemporaryFile snapshotFile(QDir(destinationDirectory).filePath(
        QStringLiteral(".%1.XXXXXX.tmp").arg(destinationInfo.fileName())));
    snapshotFile.setAutoRemove(false);
    if (!snapshotFile.open()) {
        return failure(
            SqliteBackupError::IoFailure,
            snapshotFile.errorString());
    }
    const QString snapshotPath = snapshotFile.fileName();
    if (!snapshotFile.remove()) {
        return failure(
            SqliteBackupError::IoFailure,
            QStringLiteral("Could not prepare temporary backup file"));
    }

    QString escapedPath = snapshotPath;
    escapedPath.replace(QLatin1Char('\''), QStringLiteral("''"));
    QSqlQuery snapshotQuery(m_database);
    if (!snapshotQuery.exec(
            QStringLiteral("VACUUM INTO '%1'").arg(escapedPath))) {
        QFile::remove(snapshotPath);
        return failure(
            SqliteBackupError::QueryFailure,
            snapshotQuery.lastError().text());
    }

    const SqliteBackupResult validation = validateBackup(snapshotPath);
    if (!validation.succeeded()) {
        QFile::remove(snapshotPath);
        return validation;
    }

    SqliteBackupResult copyResult = copyAtomically(
        snapshotPath, destinationInfo.absoluteFilePath());
    QFile::remove(snapshotPath);
    if (!copyResult.succeeded()) {
        return copyResult;
    }
    copyResult.schemaVersion = validation.schemaVersion;
    return copyResult;
}

SqliteBackupResult SqliteBackupService::validateBackup(
    const QString &filePath)
{
    const QFileInfo fileInfo(filePath.trimmed());
    if (!fileInfo.exists() || !fileInfo.isFile() || fileInfo.size() <= 0) {
        return failure(
            SqliteBackupError::InvalidPath,
            QStringLiteral("Backup file does not exist or is empty"));
    }

    const QString connectionName = QStringLiteral("stukit-backup-validation-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const SqliteBackupResult result = [&]() -> SqliteBackupResult {
        QSqlDatabase database = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), connectionName);
        database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        database.setDatabaseName(fileInfo.absoluteFilePath());
        if (!database.open()) {
            return failure(
                SqliteBackupError::InvalidDatabase,
                database.lastError().text());
        }

        QSqlQuery integrity(database);
        if (!integrity.exec(QStringLiteral("PRAGMA quick_check"))
            || !integrity.next()
            || integrity.value(0).toString().compare(
                   QStringLiteral("ok"), Qt::CaseInsensitive) != 0) {
            return failure(
                SqliteBackupError::InvalidDatabase,
                integrity.lastError().text().isEmpty()
                    ? QStringLiteral("SQLite integrity check failed")
                    : integrity.lastError().text());
        }

        if (!tableExists(database, QStringLiteral("schema_migrations"))) {
            return failure(
                SqliteBackupError::InvalidDatabase,
                QStringLiteral("Missing schema_migrations table"));
        }

        QSqlQuery versionQuery(database);
        if (!versionQuery.exec(QStringLiteral(
                "SELECT MIN(version), MAX(version), COUNT(*) "
                "FROM schema_migrations"))
            || !versionQuery.next()) {
            return failure(
                SqliteBackupError::InvalidDatabase,
                versionQuery.lastError().text());
        }

        const int minimumVersion = versionQuery.value(0).toInt();
        const int version = versionQuery.value(1).toInt();
        const int migrationCount = versionQuery.value(2).toInt();
        if (version > currentSchemaVersion) {
            return SqliteBackupResult{
                .error = SqliteBackupError::UnsupportedVersion,
                .detail = QStringLiteral("Backup schema is newer than this app"),
                .schemaVersion = version,
            };
        }
        if (minimumVersion != 1 || version < 1
            || migrationCount != version) {
            return failure(
                SqliteBackupError::InvalidDatabase,
                QStringLiteral("Invalid migration history"));
        }

        const bool hasRequiredTables =
            tableExists(database, QStringLiteral("accounts"))
            && (version < 2
                || tableExists(database, QStringLiteral("categories")))
            && (version < 3
                || tableExists(database, QStringLiteral("transactions")));
        if (!hasRequiredTables) {
            return failure(
                SqliteBackupError::InvalidDatabase,
                QStringLiteral("Backup is missing required tables"));
        }

        return SqliteBackupResult{
            .schemaVersion = version,
        };
    }();

    QSqlDatabase::removeDatabase(connectionName);
    return result;
}

SqliteBackupResult SqliteBackupService::replaceDatabase(
    const QString &backupPath,
    const QString &databasePath)
{
    if (pathsReferToSameFile(backupPath, databasePath)) {
        return failure(
            SqliteBackupError::SameFile,
            QStringLiteral("Backup and active database are the same file"));
    }

    const SqliteBackupResult validation = validateBackup(backupPath);
    if (!validation.succeeded()) {
        return validation;
    }

    SqliteBackupResult copyResult = copyAtomically(
        QFileInfo(backupPath).absoluteFilePath(),
        QFileInfo(databasePath).absoluteFilePath());
    copyResult.schemaVersion = validation.schemaVersion;
    return copyResult;
}

bool SqliteBackupService::pathsReferToSameFile(
    const QString &left,
    const QString &right)
{
    if (left.trimmed().isEmpty() || right.trimmed().isEmpty()) {
        return false;
    }

    const QString leftPath = QDir::cleanPath(QFileInfo(left).absoluteFilePath());
    const QString rightPath = QDir::cleanPath(QFileInfo(right).absoluteFilePath());
#ifdef Q_OS_WIN
    return leftPath.compare(rightPath, Qt::CaseInsensitive) == 0;
#else
    return leftPath == rightPath;
#endif
}

SqliteBackupResult SqliteBackupService::copyAtomically(
    const QString &sourcePath,
    const QString &destinationPath)
{
    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) {
        return failure(SqliteBackupError::IoFailure, source.errorString());
    }

    QSaveFile destination(destinationPath);
    if (!destination.open(QIODevice::WriteOnly)) {
        return failure(
            SqliteBackupError::IoFailure,
            destination.errorString());
    }

    while (!source.atEnd()) {
        const QByteArray chunk = source.read(64 * 1024);
        if (chunk.isEmpty() && source.error() != QFileDevice::NoError) {
            destination.cancelWriting();
            return failure(SqliteBackupError::IoFailure, source.errorString());
        }
        if (destination.write(chunk) != chunk.size()) {
            destination.cancelWriting();
            return failure(
                SqliteBackupError::IoFailure,
                destination.errorString());
        }
    }

    if (!destination.commit()) {
        return failure(
            SqliteBackupError::IoFailure,
            destination.errorString());
    }
    return {};
}

} // namespace stukit::accounting
