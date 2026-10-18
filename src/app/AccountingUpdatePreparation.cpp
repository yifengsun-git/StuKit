#include "AccountingUpdatePreparation.h"

#include "AccountingModule.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

#include <utility>

namespace stukit::app {

AccountingUpdatePreparation::AccountingUpdatePreparation(
    AccountingModule &accountingModule,
    QString appDataDirectory)
    : m_accountingModule(accountingModule)
    , m_appDataDirectory(std::move(appDataDirectory))
{
}

bool AccountingUpdatePreparation::prepare(
    const QString &targetVersion,
    QString &backupPath,
    QString &errorMessage)
{
    backupPath.clear();
    errorMessage.clear();
    const QFileInfo database(m_accountingModule.databasePath());
    if (!database.exists() || !database.isFile()) {
        return true;
    }

    const QString backupDirectory = QDir(m_appDataDirectory).filePath(
        QStringLiteral("backups"));
    if (!QDir().mkpath(backupDirectory)) {
        errorMessage = QStringLiteral("无法创建备份目录");
        return false;
    }

    QString safeVersion = targetVersion;
    safeVersion.replace(QLatin1Char('/'), QLatin1Char('-'));
    safeVersion.replace(QLatin1Char('\\'), QLatin1Char('-'));
    const QString timestamp = QDateTime::currentDateTime().toString(
        QStringLiteral("yyyyMMdd-HHmmss"));
    backupPath = QDir(backupDirectory).filePath(
        QStringLiteral("before-update-to-%1-%2.sqlite")
            .arg(safeVersion, timestamp));
    if (!m_accountingModule.backupDatabase(backupPath)) {
        errorMessage = m_accountingModule.lastError();
        backupPath.clear();
        return false;
    }
    return true;
}

} // namespace stukit::app
