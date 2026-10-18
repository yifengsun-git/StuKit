#include "ApplicationStorage.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

using namespace stukit::app;

namespace {

void writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
    QCOMPARE(file.write(contents), contents.size());
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

} // namespace

class ApplicationStorageTest final : public QObject
{
    Q_OBJECT

private slots:
    void resolvesInstallerManagedSiblingDirectory();
    void environmentOverrideHasPriority();
    void migratesLegacyFilesWithoutOverwritingNewData();
    void migratesMalformedInstallerDirectoryBeforeLegacyData();
    void migratesMissingSettingsWithoutOverwritingNewValues();
};

void ApplicationStorageTest::resolvesInstallerManagedSiblingDirectory()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath())
                                .filePath(QStringLiteral(
                                    "stukit-storage-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString applicationRoot = QDir(directory.path()).filePath(
        QStringLiteral("StuKit"));
    const QString applicationBin = QDir(applicationRoot).filePath(
        QStringLiteral("bin"));
    QVERIFY(QDir().mkpath(applicationBin));

    QSettings marker(
        QDir(applicationRoot).filePath(QStringLiteral("storage.ini")),
        QSettings::IniFormat);
    marker.setValue(
        QStringLiteral("Storage/DataDirectory"),
        QStringLiteral("../data"));
    marker.sync();
    QCOMPARE(marker.status(), QSettings::NoError);

    const QString legacyDirectory = QDir(directory.path()).filePath(
        QStringLiteral("legacy"));
    const auto location = ApplicationStorage::resolve(
        applicationBin, legacyDirectory);

    QCOMPARE(
        QDir::cleanPath(location.dataDirectory),
        QDir::cleanPath(QDir(directory.path()).filePath(
            QStringLiteral("data"))));
    QCOMPARE(
        QDir::cleanPath(location.legacyDataDirectory),
        QDir::cleanPath(legacyDirectory));
    QVERIFY(location.migrateLegacyData);
    QVERIFY(location.migrateLegacySettings);
}

void ApplicationStorageTest::environmentOverrideHasPriority()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath())
                                .filePath(QStringLiteral(
                                    "stukit-storage-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString overrideDirectory = QDir(directory.path()).filePath(
        QStringLiteral("override"));

    const auto location = ApplicationStorage::resolve(
        QDir(directory.path()).filePath(QStringLiteral("StuKit/bin")),
        QDir(directory.path()).filePath(QStringLiteral("legacy")),
        overrideDirectory);

    QCOMPARE(
        QDir::cleanPath(location.dataDirectory),
        QDir::cleanPath(overrideDirectory));
    QVERIFY(!location.migrateLegacyData);
    QVERIFY(!location.migrateLegacySettings);
}

void ApplicationStorageTest::migratesLegacyFilesWithoutOverwritingNewData()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath())
                                .filePath(QStringLiteral(
                                    "stukit-storage-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString legacyDirectory = QDir(directory.path()).filePath(
        QStringLiteral("legacy"));
    const QString dataDirectory = QDir(directory.path()).filePath(
        QStringLiteral("data"));
    QVERIFY(QDir().mkpath(QDir(legacyDirectory).filePath(
        QStringLiteral("backups"))));
    QVERIFY(QDir().mkpath(dataDirectory));

    writeFile(
        QDir(legacyDirectory).filePath(QStringLiteral("stukit.sqlite")),
        QByteArrayLiteral("legacy database"));
    writeFile(
        QDir(legacyDirectory).filePath(
            QStringLiteral("weather-cache-v1.json")),
        QByteArrayLiteral("legacy cache"));
    writeFile(
        QDir(legacyDirectory).filePath(
            QStringLiteral("backups/backup.sqlite")),
        QByteArrayLiteral("backup"));
    writeFile(
        QDir(dataDirectory).filePath(QStringLiteral("stukit.sqlite")),
        QByteArrayLiteral("current database"));

    ApplicationStorageLocation location{
        .dataDirectory = dataDirectory,
        .legacyDataDirectory = legacyDirectory,
        .migrateLegacyData = true,
        .migrateLegacySettings = true,
    };
    QString errorMessage;
    QVERIFY2(
        ApplicationStorage::prepareDataDirectory(location, errorMessage),
        qPrintable(errorMessage));

    QCOMPARE(
        readFile(QDir(dataDirectory).filePath(
            QStringLiteral("stukit.sqlite"))),
        QByteArrayLiteral("current database"));
    QCOMPARE(
        readFile(QDir(dataDirectory).filePath(
            QStringLiteral("weather-cache-v1.json"))),
        QByteArrayLiteral("legacy cache"));
    QCOMPARE(
        readFile(QDir(dataDirectory).filePath(
            QStringLiteral("backups/backup.sqlite"))),
        QByteArrayLiteral("backup"));

    const QString migratedCachePath = QDir(dataDirectory).filePath(
        QStringLiteral("weather-cache-v1.json"));
    QVERIFY(QFile::remove(migratedCachePath));
    QVERIFY2(
        ApplicationStorage::prepareDataDirectory(location, errorMessage),
        qPrintable(errorMessage));
    QVERIFY(!QFileInfo::exists(migratedCachePath));
}

void ApplicationStorageTest::migratesMalformedInstallerDirectoryBeforeLegacyData()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath())
                                .filePath(QStringLiteral(
                                    "stukit-storage-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString malformedDirectory = QDir(directory.path()).filePath(
        QStringLiteral("StuKit/..ata"));
    const QString legacyDirectory = QDir(directory.path()).filePath(
        QStringLiteral("legacy"));
    const QString dataDirectory = QDir(directory.path()).filePath(
        QStringLiteral("data"));
    QVERIFY(QDir().mkpath(malformedDirectory));
    QVERIFY(QDir().mkpath(legacyDirectory));

    writeFile(
        QDir(malformedDirectory).filePath(QStringLiteral("stukit.sqlite")),
        QByteArrayLiteral("installer database"));
    writeFile(
        QDir(legacyDirectory).filePath(QStringLiteral("stukit.sqlite")),
        QByteArrayLiteral("older legacy database"));
    writeFile(
        QDir(legacyDirectory).filePath(QStringLiteral("settings.ini")),
        QByteArrayLiteral("legacy settings"));

    ApplicationStorageLocation location{
        .dataDirectory = dataDirectory,
        .legacyDataDirectory = legacyDirectory,
        .malformedInstallerDataDirectory = malformedDirectory,
        .migrateLegacyData = true,
        .migrateLegacySettings = true,
    };
    QString errorMessage;
    QVERIFY2(
        ApplicationStorage::prepareDataDirectory(location, errorMessage),
        qPrintable(errorMessage));

    QCOMPARE(
        readFile(QDir(dataDirectory).filePath(
            QStringLiteral("stukit.sqlite"))),
        QByteArrayLiteral("installer database"));
    QCOMPARE(
        readFile(QDir(dataDirectory).filePath(
            QStringLiteral("settings.ini"))),
        QByteArrayLiteral("legacy settings"));
}

void ApplicationStorageTest::migratesMissingSettingsWithoutOverwritingNewValues()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath())
                                .filePath(QStringLiteral(
                                    "stukit-storage-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings legacySettings(
        QDir(directory.path()).filePath(QStringLiteral("legacy.ini")),
        QSettings::IniFormat);
    QSettings settings(
        QDir(directory.path()).filePath(QStringLiteral("settings.ini")),
        QSettings::IniFormat);
    legacySettings.setValue(
        QStringLiteral("accounting/timeZoneId"),
        QStringLiteral("Asia/Shanghai"));
    legacySettings.setValue(
        QStringLiteral("weather/location/name"),
        QStringLiteral("旧地点"));
    settings.setValue(
        QStringLiteral("weather/location/name"),
        QStringLiteral("新地点"));

    QString errorMessage;
    QVERIFY2(
        ApplicationStorage::migrateSettings(
            legacySettings, settings, errorMessage),
        qPrintable(errorMessage));
    QCOMPARE(
        settings.value(QStringLiteral("accounting/timeZoneId")).toString(),
        QStringLiteral("Asia/Shanghai"));
    QCOMPARE(
        settings.value(QStringLiteral("weather/location/name")).toString(),
        QStringLiteral("新地点"));

    settings.remove(QStringLiteral("accounting/timeZoneId"));
    legacySettings.setValue(
        QStringLiteral("accounting/timeZoneId"),
        QStringLiteral("Asia/Tokyo"));
    QVERIFY2(
        ApplicationStorage::migrateSettings(
            legacySettings, settings, errorMessage),
        qPrintable(errorMessage));
    QVERIFY(!settings.contains(QStringLiteral("accounting/timeZoneId")));
}

QTEST_MAIN(ApplicationStorageTest)

#include "ApplicationStorageTest.moc"
