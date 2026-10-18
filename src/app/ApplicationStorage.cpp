#include "ApplicationStorage.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

namespace stukit::app {
namespace {

constexpr auto StorageFileName = "storage.ini";
constexpr auto DataDirectoryKey = "Storage/DataDirectory";
constexpr auto SettingsFileName = "settings.ini";
constexpr auto DataMigrationMarkerFileName =
    ".stukit-legacy-data-migrated-v1";
constexpr auto SettingsMigrationMarkerKey =
    "Storage/LegacySettingsMigratedV1";
constexpr auto MalformedInstallerDataDirectoryName = "..ata";

QString absoluteCleanPath(const QString &path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

bool samePath(const QString &left, const QString &right)
{
#ifdef Q_OS_WIN
    constexpr auto pathCaseSensitivity = Qt::CaseInsensitive;
#else
    constexpr auto pathCaseSensitivity = Qt::CaseSensitive;
#endif
    return QDir::cleanPath(left).compare(
               QDir::cleanPath(right), pathCaseSensitivity) == 0;
}

bool copyFileIfMissing(
    const QString &sourcePath,
    const QString &destinationPath,
    QString &errorMessage)
{
    if (QFileInfo::exists(destinationPath)) {
        return true;
    }

    const QString temporaryPath = destinationPath
        + QStringLiteral(".stukit-migration");
    QFile::remove(temporaryPath);
    if (!QFile::copy(sourcePath, temporaryPath)) {
        errorMessage = QStringLiteral("无法复制旧数据文件：%1")
                           .arg(sourcePath);
        return false;
    }
    if (!QFile::rename(temporaryPath, destinationPath)) {
        QFile::remove(temporaryPath);
        errorMessage = QStringLiteral("无法安装迁移后的数据文件：%1")
                           .arg(destinationPath);
        return false;
    }
    return true;
}

bool copyDirectoryContents(
    const QString &sourceDirectory,
    const QString &destinationDirectory,
    QString &errorMessage)
{
    const QDir source(sourceDirectory);
    if (!source.exists()) {
        return true;
    }
    if (!QDir().mkpath(destinationDirectory)) {
        errorMessage = QStringLiteral("无法创建数据目录：%1")
                           .arg(destinationDirectory);
        return false;
    }

    const QFileInfoList entries = source.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden
            | QDir::System,
        QDir::DirsFirst | QDir::Name);
    for (const QFileInfo &entry : entries) {
        const QString destinationPath = QDir(destinationDirectory).filePath(
            entry.fileName());
        if (entry.isSymbolicLink()) {
            continue;
        }
        if (entry.isDir()) {
            if (!copyDirectoryContents(
                    entry.absoluteFilePath(), destinationPath, errorMessage)) {
                return false;
            }
            continue;
        }
        if (entry.isFile()
            && !copyFileIfMissing(
                entry.absoluteFilePath(), destinationPath, errorMessage)) {
            return false;
        }
    }
    return true;
}

} // namespace

ApplicationStorageLocation ApplicationStorage::resolve(
    const QString &applicationDirectory,
    const QString &standardDataDirectory,
    const QString &environmentOverride)
{
    ApplicationStorageLocation location;
    location.legacyDataDirectory = absoluteCleanPath(standardDataDirectory);

    if (!environmentOverride.trimmed().isEmpty()) {
        location.dataDirectory = absoluteCleanPath(environmentOverride);
        return location;
    }

    const QDir applicationBinDirectory(applicationDirectory);
    const QString applicationRoot = absoluteCleanPath(
        applicationBinDirectory.filePath(QStringLiteral("..")));
    const QString storageFilePath = QDir(applicationRoot).filePath(
        QString::fromLatin1(StorageFileName));
    if (QFileInfo::exists(storageFilePath)) {
        QSettings storageFile(storageFilePath, QSettings::IniFormat);
        const QString configuredDirectory = storageFile.value(
            QString::fromLatin1(DataDirectoryKey)).toString().trimmed();
        if (!configuredDirectory.isEmpty()) {
            location.dataDirectory = QFileInfo(configuredDirectory).isAbsolute()
                ? absoluteCleanPath(configuredDirectory)
                : absoluteCleanPath(
                    QDir(applicationRoot).filePath(configuredDirectory));
            location.malformedInstallerDataDirectory = absoluteCleanPath(
                QDir(applicationRoot).filePath(
                    QString::fromLatin1(
                        MalformedInstallerDataDirectoryName)));
            location.migrateLegacyData = !samePath(
                location.dataDirectory, location.legacyDataDirectory);
            location.migrateLegacySettings = true;
            return location;
        }
    }

    location.dataDirectory = location.legacyDataDirectory;
    location.migrateLegacySettings = true;
    return location;
}

QString ApplicationStorage::settingsPath(const QString &dataDirectory)
{
    return QDir(dataDirectory).filePath(
        QString::fromLatin1(SettingsFileName));
}

bool ApplicationStorage::prepareDataDirectory(
    const ApplicationStorageLocation &location,
    QString &errorMessage)
{
    errorMessage.clear();
    if (location.dataDirectory.trimmed().isEmpty()) {
        errorMessage = QStringLiteral("数据目录不能为空");
        return false;
    }
    if (!QDir().mkpath(location.dataDirectory)) {
        errorMessage = QStringLiteral("无法创建数据目录：%1")
                           .arg(location.dataDirectory);
        return false;
    }
    if (!location.migrateLegacyData) {
        return true;
    }

    const QString markerPath = QDir(location.dataDirectory).filePath(
        QString::fromLatin1(DataMigrationMarkerFileName));
    if (QFileInfo::exists(markerPath)) {
        return true;
    }
    const QStringList migrationSources{
        location.malformedInstallerDataDirectory,
        location.legacyDataDirectory,
    };
    for (const QString &sourceDirectory : migrationSources) {
        if (sourceDirectory.trimmed().isEmpty()
            || samePath(sourceDirectory, location.dataDirectory)) {
            continue;
        }
        if (!copyDirectoryContents(
                sourceDirectory,
                location.dataDirectory,
                errorMessage)) {
            return false;
        }
    }

    QFile marker(markerPath);
    const QByteArray markerContents = QByteArrayLiteral("completed\n");
    if (!marker.open(QIODevice::WriteOnly | QIODevice::Truncate)
        || marker.write(markerContents) != markerContents.size()
        || !marker.flush()) {
        errorMessage = QStringLiteral("无法记录旧数据迁移状态：%1")
                           .arg(markerPath);
        return false;
    }
    return true;
}

bool ApplicationStorage::migrateSettings(
    QSettings &legacySettings,
    QSettings &settings,
    QString &errorMessage)
{
    errorMessage.clear();
    const QString migrationMarkerKey = QString::fromLatin1(
        SettingsMigrationMarkerKey);
    if (settings.value(migrationMarkerKey).toBool()) {
        return true;
    }

    const QStringList keys = legacySettings.allKeys();
    for (const QString &key : keys) {
        if (!settings.contains(key)) {
            settings.setValue(key, legacySettings.value(key));
        }
    }
    settings.setValue(migrationMarkerKey, true);
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        errorMessage = QStringLiteral("无法写入数据目录中的设置文件：%1")
                           .arg(settings.fileName());
        return false;
    }
    return true;
}

} // namespace stukit::app
