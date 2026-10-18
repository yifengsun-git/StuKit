#pragma once

#include <QString>

class QSettings;

namespace stukit::app {

struct ApplicationStorageLocation
{
    QString dataDirectory;
    QString legacyDataDirectory;
    bool migrateLegacyData = false;
    bool migrateLegacySettings = false;
};

class ApplicationStorage final
{
public:
    [[nodiscard]] static ApplicationStorageLocation resolve(
        const QString &applicationDirectory,
        const QString &standardDataDirectory,
        const QString &environmentOverride = {});

    [[nodiscard]] static QString settingsPath(
        const QString &dataDirectory);

    [[nodiscard]] static bool prepareDataDirectory(
        const ApplicationStorageLocation &location,
        QString &errorMessage);

    [[nodiscard]] static bool migrateSettings(
        QSettings &legacySettings,
        QSettings &settings,
        QString &errorMessage);
};

} // namespace stukit::app
