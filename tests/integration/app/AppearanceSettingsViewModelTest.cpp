#include "AppearanceSettingsViewModel.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QVariantMap>

#include <miniz.h>

using stukit::app::AppearanceSettingsViewModel;

namespace
{

QString writeThemeFile(const QString &path, const QVariantMap &definition)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return {};
    }
    file.write(
        QJsonDocument(QJsonObject::fromVariantMap(definition)).toJson(QJsonDocument::Indented));
    file.close();
    return path;
}

QString writeThemeArchive(const QString &path, const QVariantMap &definition,
                          const QMap<QString, QByteArray> &resources)
{
    mz_zip_archive archive{};
    mz_zip_zero_struct(&archive);
    if (!mz_zip_writer_init_heap(&archive, 0, 128 * 1024))
    {
        return {};
    }

    const QByteArray json =
        QJsonDocument(QJsonObject::fromVariantMap(definition)).toJson(QJsonDocument::Indented);
    bool succeeded = mz_zip_writer_add_mem(&archive, "theme.json", json.constData(),
                                           static_cast<size_t>(json.size()), MZ_BEST_COMPRESSION);
    for (auto iterator = resources.cbegin(); succeeded && iterator != resources.cend(); ++iterator)
    {
        const QByteArray name = iterator.key().toUtf8();
        succeeded = mz_zip_writer_add_mem(&archive, name.constData(), iterator.value().constData(),
                                          static_cast<size_t>(iterator.value().size()),
                                          MZ_BEST_COMPRESSION);
    }

    void *archiveData = nullptr;
    size_t archiveSize = 0;
    succeeded =
        succeeded && mz_zip_writer_finalize_heap_archive(&archive, &archiveData, &archiveSize);
    QByteArray contents;
    if (succeeded)
    {
        contents =
            QByteArray(static_cast<const char *>(archiveData), static_cast<qsizetype>(archiveSize));
    }
    mz_free(archiveData);
    mz_zip_writer_end(&archive);

    if (!succeeded)
    {
        return {};
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size())
    {
        return {};
    }
    return path;
}

QVariantMap customThemeFrom(const AppearanceSettingsViewModel &viewModel)
{
    QVariantMap definition = viewModel.themes().constFirst().toMap();
    definition.insert(QStringLiteral("themeId"), QStringLiteral("student.green"));
    definition.insert(QStringLiteral("name"), QStringLiteral("学生绿"));
    definition.insert(QStringLiteral("description"), QStringLiteral("测试用第三方主题"));
    definition.insert(QStringLiteral("author"), QStringLiteral("Tester"));
    definition.insert(QStringLiteral("version"), QStringLiteral("1.0.0"));
    definition.insert(QStringLiteral("builtIn"), false);
    return definition;
}

QByteArray onePixelPng()
{
    return QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+"
                                  "A8AAQUBAScY42YAAAAASUVORK5CYII=");
}

} // namespace

class AppearanceSettingsViewModelTest final : public QObject
{
    Q_OBJECT

private slots:
    void exposesBuiltInThemesAndPersistsSelection();
    void rejectsUnknownThemesAndRecoversInvalidSettings();
    void importsPersistsAndRemovesCustomTheme();
    void previewsAndCancelsThemeBeforeInstallation();
    void installsArchiveThemeWithPreviewResource();
    void rejectsUnsafeThemePackage();
    void rejectsArchivePathTraversal();
    void rejectsInvalidImageAndUnsafeSvgResources();
};

void AppearanceSettingsViewModelTest::exposesBuiltInThemesAndPersistsSelection()
{
    QTemporaryDir directory(
        QDir::current().filePath(QStringLiteral("stukit-appearance-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString settingsPath = directory.filePath(QStringLiteral("settings.ini"));
    QSettings settings(settingsPath, QSettings::IniFormat);
    const QString themesDirectory = directory.filePath(QStringLiteral("themes"));
    AppearanceSettingsViewModel viewModel(settings, themesDirectory);

    QCOMPARE(viewModel.themeId(), QStringLiteral("default"));
    QCOMPARE(viewModel.themes().size(), 3);
    QCOMPARE(viewModel.themes().at(2).toMap().value(QStringLiteral("themeId")).toString(),
             QStringLiteral("cat-warm"));
    const QVariantList diagnostics = viewModel.themeDiagnostics(QStringLiteral("default"));
    QVERIFY(diagnostics.size() >= 7);
    for (const QVariant &entry : diagnostics)
    {
        QVERIFY(entry.toMap().contains(QStringLiteral("passed")));
        QVERIFY(entry.toMap().contains(QStringLiteral("detail")));
    }
    QVERIFY(!viewModel.exportTheme(
        QStringLiteral("default"),
        QUrl::fromLocalFile(directory.filePath(QStringLiteral("default-backup")))));
    QVERIFY(viewModel.hasError());
    viewModel.clearMessages();

    QSignalSpy changedSpy(&viewModel, &AppearanceSettingsViewModel::themeIdChanged);
    QVERIFY(viewModel.selectTheme(QStringLiteral("dark")));
    QCOMPARE(viewModel.themeId(), QStringLiteral("dark"));
    QCOMPARE(viewModel.currentTheme().value(QStringLiteral("backgroundColor")).toString(),
             QStringLiteral("#111827"));
    QCOMPARE(changedSpy.count(), 1);
    settings.sync();

    QSettings restoredSettings(settingsPath, QSettings::IniFormat);
    AppearanceSettingsViewModel restored(restoredSettings, themesDirectory);
    QCOMPARE(restored.themeId(), QStringLiteral("dark"));
}

void AppearanceSettingsViewModelTest::rejectsUnknownThemesAndRecoversInvalidSettings()
{
    QTemporaryDir directory(
        QDir::current().filePath(QStringLiteral("stukit-appearance-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString settingsPath = directory.filePath(QStringLiteral("settings.ini"));
    QSettings settings(settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("appearance/themeId"), QStringLiteral("missing-theme"));

    AppearanceSettingsViewModel viewModel(settings, directory.filePath(QStringLiteral("themes")));
    QCOMPARE(viewModel.themeId(), QStringLiteral("default"));
    QVERIFY(!viewModel.selectTheme(QStringLiteral("unknown")));
    QCOMPARE(viewModel.themeId(), QStringLiteral("default"));
}

void AppearanceSettingsViewModelTest::importsPersistsAndRemovesCustomTheme()
{
    QTemporaryDir directory(
        QDir::current().filePath(QStringLiteral("stukit-appearance-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString settingsPath = directory.filePath(QStringLiteral("settings.ini"));
    const QString themesDirectory = directory.filePath(QStringLiteral("themes"));
    QSettings settings(settingsPath, QSettings::IniFormat);
    AppearanceSettingsViewModel viewModel(settings, themesDirectory);
    const QVariantMap definition = customThemeFrom(viewModel);
    const QString packagePath = writeThemeFile(
        directory.filePath(QStringLiteral("student-green.stukit-theme")), definition);
    QVERIFY(!packagePath.isEmpty());

    QSignalSpy themesSpy(&viewModel, &AppearanceSettingsViewModel::themesChanged);
    QVERIFY(viewModel.inspectTheme(QUrl::fromLocalFile(packagePath)));
    QVERIFY(viewModel.hasPendingTheme());
    QCOMPARE(viewModel.pendingTheme().value(QStringLiteral("name")).toString(),
             QStringLiteral("学生绿"));
    QVERIFY(!viewModel.pendingThemeReplacesExisting());
    QCOMPARE(viewModel.themes().size(), 3);
    QVERIFY(!QFile::exists(directory.filePath(QStringLiteral("themes/student.green/theme.json"))));

    QVERIFY(viewModel.installPendingTheme());
    QVERIFY(!viewModel.hasPendingTheme());
    QCOMPARE(viewModel.themes().size(), 4);
    QCOMPARE(viewModel.themeId(), QStringLiteral("student.green"));
    QCOMPARE(themesSpy.count(), 1);
    QVERIFY(QFile::exists(directory.filePath(QStringLiteral("themes/student.green/theme.json"))));
    settings.sync();

    QSettings restoredSettings(settingsPath, QSettings::IniFormat);
    AppearanceSettingsViewModel restored(restoredSettings, themesDirectory);
    QCOMPARE(restored.themes().size(), 4);
    QCOMPARE(restored.themeId(), QStringLiteral("student.green"));

    QVERIFY2(restored.removeTheme(QStringLiteral("student.green")),
             qPrintable(restored.errorMessage()));
    QCOMPARE(restored.themes().size(), 3);
    QCOMPARE(restored.themeId(), QStringLiteral("default"));
    QVERIFY(!QFile::exists(directory.filePath(QStringLiteral("themes/student.green/theme.json"))));
}

void AppearanceSettingsViewModelTest::previewsAndCancelsThemeBeforeInstallation()
{
    QTemporaryDir directory(
        QDir::current().filePath(QStringLiteral("stukit-appearance-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")), QSettings::IniFormat);
    AppearanceSettingsViewModel viewModel(settings, directory.filePath(QStringLiteral("themes")));
    const QString packagePath = writeThemeFile(
        directory.filePath(QStringLiteral("preview.stukit-theme")), customThemeFrom(viewModel));
    QVERIFY(!packagePath.isEmpty());

    QSignalSpy pendingSpy(&viewModel, &AppearanceSettingsViewModel::pendingThemeChanged);
    QVERIFY(viewModel.inspectTheme(QUrl::fromLocalFile(packagePath)));
    QVERIFY(viewModel.hasPendingTheme());
    QCOMPARE(pendingSpy.count(), 1);

    viewModel.cancelThemeImport();
    QVERIFY(!viewModel.hasPendingTheme());
    QCOMPARE(pendingSpy.count(), 2);
    QCOMPARE(viewModel.themes().size(), 3);
    QVERIFY(!QFile::exists(directory.filePath(QStringLiteral("themes/student.green/theme.json"))));
    QVERIFY(!viewModel.installPendingTheme());
    QVERIFY(viewModel.hasError());
}

void AppearanceSettingsViewModelTest::installsArchiveThemeWithPreviewResource()
{
    QTemporaryDir directory(
        QDir::current().filePath(QStringLiteral("stukit-appearance-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString settingsPath = directory.filePath(QStringLiteral("settings.ini"));
    const QString themesDirectory = directory.filePath(QStringLiteral("themes"));
    QSettings settings(settingsPath, QSettings::IniFormat);
    AppearanceSettingsViewModel viewModel(settings, themesDirectory);
    QVariantMap definition = customThemeFrom(viewModel);
    definition.insert(QStringLiteral("previewImage"), QStringLiteral("preview.png"));
    definition.insert(QStringLiteral("backgroundImage"), QStringLiteral("assets/background.png"));
    definition.insert(QStringLiteral("backgroundImageOpacity"), 0.1);
    definition.insert(QStringLiteral("navigationLogo"), QStringLiteral("assets/logo.svg"));
    definition.insert(QStringLiteral("icons"),
                      QVariantMap{{QStringLiteral("home"), QStringLiteral("assets/home.svg")}});
    const QByteArray preview = onePixelPng();
    const QByteArray svg =
        QByteArrayLiteral("<svg viewBox=\"0 0 10 10\"><path d=\"M0 0h10v10H0z\"/></svg>");
    const QString packagePath = writeThemeArchive(
        directory.filePath(QStringLiteral("student-green.stukit-theme")), definition,
        {{QStringLiteral("preview.png"), preview},
         {QStringLiteral("assets/background.png"), preview},
         {QStringLiteral("assets/logo.svg"), svg},
         {QStringLiteral("assets/home.svg"), svg}});
    QVERIFY(!packagePath.isEmpty());

    QVERIFY2(viewModel.inspectTheme(QUrl::fromLocalFile(packagePath)),
             qPrintable(viewModel.errorMessage()));
    QCOMPARE(viewModel.pendingTheme().value(QStringLiteral("packageFormat")).toString(),
             QStringLiteral("archive"));
    QVERIFY(viewModel.pendingTheme()
                .value(QStringLiteral("previewUrl"))
                .toString()
                .startsWith(QStringLiteral("data:image/png;base64,")));
    QVERIFY(viewModel.pendingTheme()
                .value(QStringLiteral("backgroundImageUrl"))
                .toString()
                .startsWith(QStringLiteral("data:image/png;base64,")));
    QVERIFY(viewModel.pendingTheme()
                .value(QStringLiteral("navigationLogoUrl"))
                .toString()
                .startsWith(QStringLiteral("data:image/svg+xml;base64,")));
    QVERIFY(viewModel.pendingTheme()
                .value(QStringLiteral("navigationIconUrls"))
                .toMap()
                .value(QStringLiteral("home"))
                .toString()
                .startsWith(QStringLiteral("data:image/svg+xml;base64,")));
    QVERIFY(viewModel.installPendingTheme());
    QVERIFY(QFile::exists(directory.filePath(QStringLiteral("themes/student.green/preview.png"))));
    QVERIFY(
        QFile::exists(directory.filePath(QStringLiteral("themes/student.green/assets/home.svg"))));

    settings.sync();
    QSettings restoredSettings(settingsPath, QSettings::IniFormat);
    AppearanceSettingsViewModel restored(restoredSettings, themesDirectory);
    const QVariantMap restoredTheme = restored.currentTheme();
    QCOMPARE(restoredTheme.value(QStringLiteral("previewImage")).toString(),
             QStringLiteral("preview.png"));
    QVERIFY(restoredTheme.value(QStringLiteral("previewUrl")).toUrl().isLocalFile());
    QVERIFY(restoredTheme.value(QStringLiteral("backgroundImageUrl")).toUrl().isLocalFile());
    QVERIFY(restoredTheme.value(QStringLiteral("navigationLogoUrl")).toUrl().isLocalFile());
    QVERIFY(restoredTheme.value(QStringLiteral("navigationIconUrls"))
                .toMap()
                .value(QStringLiteral("home"))
                .toUrl()
                .isLocalFile());

    const QVariantList diagnostics = restored.themeDiagnostics(QStringLiteral("student.green"));
    QVERIFY(diagnostics.size() >= 10);
    for (const QVariant &entry : diagnostics)
    {
        const QVariantMap result = entry.toMap();
        if (result.value(QStringLiteral("label")).toString().contains(QStringLiteral("图")) ||
            result.value(QStringLiteral("label")).toString().contains(QStringLiteral("背景")))
        {
            QVERIFY(result.value(QStringLiteral("passed")).toBool());
        }
    }

    const QString exportBase = directory.filePath(QStringLiteral("student-green-backup"));
    QVERIFY2(restored.exportTheme(QStringLiteral("student.green"), QUrl::fromLocalFile(exportBase)),
             qPrintable(restored.errorMessage()));
    const QString exportedPackage = exportBase + QStringLiteral(".stukit-theme");
    QVERIFY(QFile::exists(exportedPackage));
    QSettings importSettings(directory.filePath(QStringLiteral("import-settings.ini")),
                             QSettings::IniFormat);
    AppearanceSettingsViewModel imported(importSettings,
                                         directory.filePath(QStringLiteral("imported-themes")));
    QVERIFY2(imported.inspectTheme(QUrl::fromLocalFile(exportedPackage)),
             qPrintable(imported.errorMessage()));
    QCOMPARE(imported.pendingTheme().value(QStringLiteral("themeId")).toString(),
             QStringLiteral("student.green"));
    QVERIFY(imported.pendingTheme()
                .value(QStringLiteral("navigationIconUrls"))
                .toMap()
                .contains(QStringLiteral("home")));

    QVERIFY(restored.removeTheme(QStringLiteral("student.green")));
    QVERIFY(!QDir(directory.filePath(QStringLiteral("themes/student.green"))).exists());
}

void AppearanceSettingsViewModelTest::rejectsUnsafeThemePackage()
{
    QTemporaryDir directory(
        QDir::current().filePath(QStringLiteral("stukit-appearance-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")), QSettings::IniFormat);
    AppearanceSettingsViewModel viewModel(settings, directory.filePath(QStringLiteral("themes")));
    QVariantMap definition = customThemeFrom(viewModel);
    definition.insert(QStringLiteral("themeId"), QStringLiteral("../unsafe"));
    const QString packagePath =
        writeThemeFile(directory.filePath(QStringLiteral("unsafe.stukit-theme")), definition);
    QVERIFY(!packagePath.isEmpty());

    QVERIFY(!viewModel.importTheme(QUrl::fromLocalFile(packagePath)));
    QVERIFY(viewModel.hasError());
    QCOMPARE(viewModel.themes().size(), 3);
    QVERIFY(!QFile::exists(directory.filePath(QStringLiteral("unsafe/theme.json"))));
}

void AppearanceSettingsViewModelTest::rejectsArchivePathTraversal()
{
    QTemporaryDir directory(
        QDir::current().filePath(QStringLiteral("stukit-appearance-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")), QSettings::IniFormat);
    AppearanceSettingsViewModel viewModel(settings, directory.filePath(QStringLiteral("themes")));
    const QString packagePath =
        writeThemeArchive(directory.filePath(QStringLiteral("unsafe-archive.stukit-theme")),
                          customThemeFrom(viewModel),
                          {{QStringLiteral("../escape.png"), QByteArrayLiteral("not-an-image")}});
    QVERIFY(!packagePath.isEmpty());

    QVERIFY(!viewModel.inspectTheme(QUrl::fromLocalFile(packagePath)));
    QVERIFY(viewModel.hasError());
    QVERIFY(!QFile::exists(directory.filePath(QStringLiteral("escape.png"))));
    QCOMPARE(viewModel.themes().size(), 3);
}

void AppearanceSettingsViewModelTest::rejectsInvalidImageAndUnsafeSvgResources()
{
    QTemporaryDir directory(
        QDir::current().filePath(QStringLiteral("stukit-appearance-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")), QSettings::IniFormat);
    AppearanceSettingsViewModel viewModel(settings, directory.filePath(QStringLiteral("themes")));
    QVariantMap definition = customThemeFrom(viewModel);
    definition.insert(QStringLiteral("previewImage"), QStringLiteral("preview.png"));

    QString packagePath =
        writeThemeArchive(directory.filePath(QStringLiteral("fake-image.stukit-theme")), definition,
                          {{QStringLiteral("preview.png"), QByteArrayLiteral("not a png")}});
    QVERIFY(!packagePath.isEmpty());
    QVERIFY(!viewModel.inspectTheme(QUrl::fromLocalFile(packagePath)));
    QVERIFY(viewModel.errorMessage().contains(QStringLiteral("图片")));

    definition.remove(QStringLiteral("previewImage"));
    definition.insert(QStringLiteral("navigationLogo"), QStringLiteral("assets/logo.svg"));
    packagePath =
        writeThemeArchive(directory.filePath(QStringLiteral("unsafe-svg.stukit-theme")), definition,
                          {{QStringLiteral("assets/logo.svg"),
                            QByteArrayLiteral("<svg><script>alert(1)</script></svg>")}});
    QVERIFY(!packagePath.isEmpty());
    QVERIFY(!viewModel.inspectTheme(QUrl::fromLocalFile(packagePath)));
    QVERIFY(viewModel.errorMessage().contains(QStringLiteral("SVG")));
    QCOMPARE(viewModel.themes().size(), 3);
}

QTEST_MAIN(AppearanceSettingsViewModelTest)

#include "AppearanceSettingsViewModelTest.moc"
