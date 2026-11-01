#include "AppearanceSettingsViewModel.h"
#include "ThemePackageReader.h"
#include "ThemePackageWriter.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QStringList>
#include <QUrl>
#include <QUuid>
#include <QVariantMap>

#include <cmath>
#include <utility>

namespace stukit::app
{
namespace
{

constexpr auto ThemeIdKey = "appearance/themeId";
constexpr auto DefaultThemeId = "default";
constexpr auto ThemeFileName = "theme.json";
constexpr qint64 MaximumThemeFileSize = 256 * 1024;

const QStringList ColorTokenNames{
    QStringLiteral("backgroundColor"),     QStringLiteral("surfaceColor"),
    QStringLiteral("surfaceRaisedColor"),  QStringLiteral("surfaceVariantColor"),
    QStringLiteral("surfaceHoverColor"),   QStringLiteral("selectionColor"),
    QStringLiteral("primaryColor"),        QStringLiteral("primaryHoverColor"),
    QStringLiteral("primaryPressedColor"), QStringLiteral("textPrimaryColor"),
    QStringLiteral("textSecondaryColor"),  QStringLiteral("borderColor"),
    QStringLiteral("borderStrongColor"),   QStringLiteral("textOnPrimaryColor"),
    QStringLiteral("errorColor"),          QStringLiteral("successColor"),
    QStringLiteral("warningColor"),
};

const QStringList NavigationIconSlots{
    QStringLiteral("home"),       QStringLiteral("accounting"), QStringLiteral("schedule"),
    QStringLiteral("tasks"),      QStringLiteral("weather"),    QStringLiteral("update"),
    QStringLiteral("appearance"),
};

bool isBuiltInThemeId(const QString &themeId)
{
    return themeId == QStringLiteral("default") || themeId == QStringLiteral("dark") ||
           themeId == QStringLiteral("cat-warm");
}

bool isValidColor(const QString &value)
{
    static const QRegularExpression colorPattern(
        QStringLiteral("^#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?$"));
    return colorPattern.match(value).hasMatch();
}

bool isSafeResourceReference(const QString &path, bool allowSvg = false)
{
    if (path.isEmpty() || path.startsWith(QLatin1Char('/')) || path.contains(QLatin1Char('\\')) ||
        path.contains(QLatin1Char(':')) || QDir::cleanPath(path) != path)
    {
        return false;
    }
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == QStringLiteral("png") || suffix == QStringLiteral("jpg") ||
           suffix == QStringLiteral("jpeg") || suffix == QStringLiteral("webp") ||
           (allowSvg && suffix == QStringLiteral("svg"));
}

QString imageMimeType(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QStringLiteral("jpg") || suffix == QStringLiteral("jpeg"))
    {
        return QStringLiteral("image/jpeg");
    }
    if (suffix == QStringLiteral("webp"))
    {
        return QStringLiteral("image/webp");
    }
    if (suffix == QStringLiteral("svg"))
    {
        return QStringLiteral("image/svg+xml");
    }
    return QStringLiteral("image/png");
}

QString imageDataUrl(const QString &path, const QByteArray &data)
{
    return QStringLiteral("data:%1;base64,%2")
        .arg(imageMimeType(path), QString::fromLatin1(data.toBase64()));
}

bool writePackageFile(const QString &path, const QByteArray &contents)
{
    const QFileInfo fileInfo(path);
    if (!QDir().mkpath(fileInfo.absolutePath()))
    {
        return false;
    }
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size() &&
           file.commit();
}

QUrl installedResourceUrl(const QString &reference, const QString &themeDirectory, bool allowSvg)
{
    if (!isSafeResourceReference(reference, allowSvg))
    {
        return {};
    }
    const QString path = QDir(themeDirectory).absoluteFilePath(reference);
    return QFileInfo(path).isFile() ? QUrl::fromLocalFile(path) : QUrl{};
}

void applyInstalledResources(QVariantMap &definition, const QString &themeDirectory)
{
    const QUrl previewUrl = installedResourceUrl(
        definition.value(QStringLiteral("previewImage")).toString(), themeDirectory, false);
    if (previewUrl.isValid())
    {
        definition.insert(QStringLiteral("previewUrl"), previewUrl);
    }

    const QUrl backgroundUrl = installedResourceUrl(
        definition.value(QStringLiteral("backgroundImage")).toString(), themeDirectory, false);
    if (backgroundUrl.isValid())
    {
        definition.insert(QStringLiteral("backgroundImageUrl"), backgroundUrl);
    }

    const QUrl logoUrl = installedResourceUrl(
        definition.value(QStringLiteral("navigationLogo")).toString(), themeDirectory, true);
    if (logoUrl.isValid())
    {
        definition.insert(QStringLiteral("navigationLogoUrl"), logoUrl);
    }

    const QVariantMap icons = definition.value(QStringLiteral("icons")).toMap();
    QVariantMap resolvedIcons;
    for (auto iterator = icons.cbegin(); iterator != icons.cend(); ++iterator)
    {
        const QUrl iconUrl =
            installedResourceUrl(iterator.value().toString(), themeDirectory, true);
        if (iconUrl.isValid())
        {
            resolvedIcons.insert(iterator.key(), iconUrl);
        }
    }
    if (!resolvedIcons.isEmpty())
    {
        definition.insert(QStringLiteral("navigationIconUrls"), resolvedIcons);
    }
}

bool applyPendingResource(QVariantMap &definition, const QMap<QString, QByteArray> &resources,
                          const QString &definitionKey, const QString &runtimeKey, bool allowSvg,
                          QString &errorMessage)
{
    const QString reference = definition.value(definitionKey).toString();
    if (reference.isEmpty())
    {
        return true;
    }
    if (!isSafeResourceReference(reference, allowSvg) || !resources.contains(reference))
    {
        errorMessage =
            QStringLiteral("%1 必须指向主题包内存在的受支持图片资源。").arg(definitionKey);
        return false;
    }
    definition.insert(runtimeKey, imageDataUrl(reference, resources.value(reference)));
    return true;
}

double linearColorComponent(double value)
{
    value /= 255.0;
    return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
}

double relativeLuminance(const QColor &color)
{
    return 0.2126 * linearColorComponent(color.red()) +
           0.7152 * linearColorComponent(color.green()) +
           0.0722 * linearColorComponent(color.blue());
}

double contrastRatio(const QString &foregroundValue, const QString &backgroundValue)
{
    const QColor foreground(foregroundValue);
    const QColor background(backgroundValue);
    if (!foreground.isValid() || !background.isValid())
    {
        return 0.0;
    }
    const double first = relativeLuminance(foreground);
    const double second = relativeLuminance(background);
    return (std::max(first, second) + 0.05) / (std::min(first, second) + 0.05);
}

QVariantMap diagnostic(const QString &label, bool passed, const QString &detail,
                       const QString &severity = QStringLiteral("warning"))
{
    return {
        {QStringLiteral("label"), label},
        {QStringLiteral("passed"), passed},
        {QStringLiteral("severity"), passed ? QStringLiteral("pass") : severity},
        {QStringLiteral("detail"), detail},
    };
}

QVariantMap theme(const char *themeId, const char *name, const char *description, bool dark,
                  const QVariantMap &tokens)
{
    QVariantMap result{
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("themeId"), QString::fromLatin1(themeId)},
        {QStringLiteral("name"), QString::fromUtf8(name)},
        {QStringLiteral("description"), QString::fromUtf8(description)},
        {QStringLiteral("dark"), dark},
        {QStringLiteral("builtIn"), true},
        {QStringLiteral("author"), QStringLiteral("StuKit")},
        {QStringLiteral("version"), QStringLiteral("1.0.0")},
    };
    for (auto iterator = tokens.cbegin(); iterator != tokens.cend(); ++iterator)
    {
        result.insert(iterator.key(), iterator.value());
    }
    return result;
}

QVariantList builtInThemes()
{
    return {
        theme("default", "StuKit 默认", "清晰、克制的蓝色桌面主题，适合长时间使用。", false,
              {
                  {QStringLiteral("backgroundColor"), QStringLiteral("#F3F4F6")},
                  {QStringLiteral("surfaceColor"), QStringLiteral("#FFFFFF")},
                  {QStringLiteral("surfaceRaisedColor"), QStringLiteral("#FFFFFF")},
                  {QStringLiteral("surfaceVariantColor"), QStringLiteral("#E5E7EB")},
                  {QStringLiteral("surfaceHoverColor"), QStringLiteral("#F8FAFC")},
                  {QStringLiteral("selectionColor"), QStringLiteral("#DBEAFE")},
                  {QStringLiteral("primaryColor"), QStringLiteral("#2563EB")},
                  {QStringLiteral("primaryHoverColor"), QStringLiteral("#1D4ED8")},
                  {QStringLiteral("primaryPressedColor"), QStringLiteral("#1D4ED8")},
                  {QStringLiteral("textPrimaryColor"), QStringLiteral("#111827")},
                  {QStringLiteral("textSecondaryColor"), QStringLiteral("#4B5563")},
                  {QStringLiteral("borderColor"), QStringLiteral("#D1D5DB")},
                  {QStringLiteral("borderStrongColor"), QStringLiteral("#94A3B8")},
                  {QStringLiteral("textOnPrimaryColor"), QStringLiteral("#FFFFFF")},
                  {QStringLiteral("errorColor"), QStringLiteral("#DC2626")},
                  {QStringLiteral("successColor"), QStringLiteral("#059669")},
                  {QStringLiteral("warningColor"), QStringLiteral("#D97706")},
                  {QStringLiteral("radiusSmall"), 6},
                  {QStringLiteral("radiusMedium"), 10},
                  {QStringLiteral("radiusLarge"), 16},
                  {QStringLiteral("navigationRailWidth"), 224},
              }),
        theme("dark", "深夜专注", "降低夜间亮度，同时保持账单和课表信息的对比度。", true,
              {
                  {QStringLiteral("backgroundColor"), QStringLiteral("#111827")},
                  {QStringLiteral("surfaceColor"), QStringLiteral("#1F2937")},
                  {QStringLiteral("surfaceRaisedColor"), QStringLiteral("#273449")},
                  {QStringLiteral("surfaceVariantColor"), QStringLiteral("#374151")},
                  {QStringLiteral("surfaceHoverColor"), QStringLiteral("#334155")},
                  {QStringLiteral("selectionColor"), QStringLiteral("#1E3A5F")},
                  {QStringLiteral("primaryColor"), QStringLiteral("#60A5FA")},
                  {QStringLiteral("primaryHoverColor"), QStringLiteral("#93C5FD")},
                  {QStringLiteral("primaryPressedColor"), QStringLiteral("#3B82F6")},
                  {QStringLiteral("textPrimaryColor"), QStringLiteral("#F9FAFB")},
                  {QStringLiteral("textSecondaryColor"), QStringLiteral("#D1D5DB")},
                  {QStringLiteral("borderColor"), QStringLiteral("#4B5563")},
                  {QStringLiteral("borderStrongColor"), QStringLiteral("#64748B")},
                  {QStringLiteral("textOnPrimaryColor"), QStringLiteral("#FFFFFF")},
                  {QStringLiteral("errorColor"), QStringLiteral("#FCA5A5")},
                  {QStringLiteral("successColor"), QStringLiteral("#6EE7B7")},
                  {QStringLiteral("warningColor"), QStringLiteral("#FCD34D")},
                  {QStringLiteral("radiusSmall"), 6},
                  {QStringLiteral("radiusMedium"), 10},
                  {QStringLiteral("radiusLarge"), 16},
                  {QStringLiteral("navigationRailWidth"), 224},
              }),
        theme("cat-warm", "肥猫勿喂", "取自 StuKit 猫咪图标的自然暖色主题，使用树叶绿和纸张米色。",
              false,
              {
                  {QStringLiteral("backgroundColor"), QStringLiteral("#F7F1E3")},
                  {QStringLiteral("surfaceColor"), QStringLiteral("#FFFDF7")},
                  {QStringLiteral("surfaceRaisedColor"), QStringLiteral("#FFFAED")},
                  {QStringLiteral("surfaceVariantColor"), QStringLiteral("#E8E2C8")},
                  {QStringLiteral("surfaceHoverColor"), QStringLiteral("#F3E8C8")},
                  {QStringLiteral("selectionColor"), QStringLiteral("#E5D4A4")},
                  {QStringLiteral("primaryColor"), QStringLiteral("#5F6F3D")},
                  {QStringLiteral("primaryHoverColor"), QStringLiteral("#4D5E30")},
                  {QStringLiteral("primaryPressedColor"), QStringLiteral("#3F4E28")},
                  {QStringLiteral("textPrimaryColor"), QStringLiteral("#332A1F")},
                  {QStringLiteral("textSecondaryColor"), QStringLiteral("#6F6251")},
                  {QStringLiteral("borderColor"), QStringLiteral("#D8C9AD")},
                  {QStringLiteral("borderStrongColor"), QStringLiteral("#A99778")},
                  {QStringLiteral("textOnPrimaryColor"), QStringLiteral("#FFFFFF")},
                  {QStringLiteral("errorColor"), QStringLiteral("#B8493D")},
                  {QStringLiteral("successColor"), QStringLiteral("#4F7A45")},
                  {QStringLiteral("warningColor"), QStringLiteral("#B36A1E")},
                  {QStringLiteral("radiusSmall"), 10},
                  {QStringLiteral("radiusMedium"), 14},
                  {QStringLiteral("radiusLarge"), 20},
                  {QStringLiteral("navigationRailWidth"), 232},
              }),
    };
}

} // namespace

AppearanceSettingsViewModel::AppearanceSettingsViewModel(QSettings &settings,
                                                         QString themesDirectory, QObject *parent)
    : QObject(parent), m_settings(settings),
      m_themesDirectory(QDir(themesDirectory).absolutePath()),
      m_themeId(QString::fromLatin1(DefaultThemeId))
{
    loadInstalledThemes();
    m_themeId = settings.value(QString::fromLatin1(ThemeIdKey), QString::fromLatin1(DefaultThemeId))
                    .toString();
    if (!supportsTheme(m_themeId))
    {
        m_themeId = QString::fromLatin1(DefaultThemeId);
        m_settings.setValue(QString::fromLatin1(ThemeIdKey), m_themeId);
    }
}

QString AppearanceSettingsViewModel::themeId() const
{
    return m_themeId;
}

QVariantList AppearanceSettingsViewModel::themes() const
{
    return m_themes;
}

QVariantMap AppearanceSettingsViewModel::currentTheme() const
{
    for (const QVariant &entry : m_themes)
    {
        const QVariantMap definition = entry.toMap();
        if (definition.value(QStringLiteral("themeId")).toString() == m_themeId)
        {
            return definition;
        }
    }
    return builtInThemes().constFirst().toMap();
}

bool AppearanceSettingsViewModel::hasError() const
{
    return !m_errorMessage.isEmpty();
}

QString AppearanceSettingsViewModel::errorMessage() const
{
    return m_errorMessage;
}

QString AppearanceSettingsViewModel::statusMessage() const
{
    return m_statusMessage;
}

bool AppearanceSettingsViewModel::hasPendingTheme() const
{
    return !m_pendingTheme.isEmpty();
}

QVariantMap AppearanceSettingsViewModel::pendingTheme() const
{
    return m_pendingTheme;
}

bool AppearanceSettingsViewModel::pendingThemeReplacesExisting() const
{
    return m_pendingThemeReplacesExisting;
}

void AppearanceSettingsViewModel::setThemeId(const QString &themeId)
{
    selectTheme(themeId);
}

bool AppearanceSettingsViewModel::selectTheme(const QString &themeId)
{
    if (!supportsTheme(themeId))
    {
        return false;
    }
    if (m_themeId == themeId)
    {
        return true;
    }

    m_themeId = themeId;
    m_settings.setValue(QString::fromLatin1(ThemeIdKey), m_themeId);
    emit themeIdChanged();
    return true;
}

bool AppearanceSettingsViewModel::inspectTheme(const QUrl &fileUrl)
{
    clearMessages();
    cancelThemeImport();
    if (!fileUrl.isLocalFile())
    {
        setError(QStringLiteral("主题文件必须来自本机。"));
        return false;
    }

    ThemePackageData package;
    QString packageError;
    if (!ThemePackageReader::read(fileUrl.toLocalFile(), package, packageError))
    {
        setError(packageError);
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(package.themeDefinition, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        setError(QStringLiteral("主题文件不是有效的 JSON 对象：%1").arg(parseError.errorString()));
        return false;
    }

    QVariantMap definition = document.object().toVariantMap();
    QString validationError;
    if (!validateTheme(definition, validationError))
    {
        setError(validationError);
        return false;
    }

    const QString importedThemeId = definition.value(QStringLiteral("themeId")).toString();
    if (isBuiltInThemeId(importedThemeId))
    {
        setError(QStringLiteral("第三方主题不能覆盖内置主题。"));
        return false;
    }

    QString previewImage = definition.value(QStringLiteral("previewImage")).toString();
    if (previewImage.isEmpty() && package.archive)
    {
        const QStringList conventionalPreviewNames{
            QStringLiteral("preview.png"), QStringLiteral("preview.jpg"),
            QStringLiteral("preview.jpeg"), QStringLiteral("preview.webp")};
        for (const QString &candidate : conventionalPreviewNames)
        {
            if (package.resources.contains(candidate))
            {
                previewImage = candidate;
                break;
            }
        }
    }
    if (!previewImage.isEmpty())
    {
        if (!package.archive || !isSafeResourceReference(previewImage) ||
            !package.resources.contains(previewImage))
        {
            setError(
                QStringLiteral("previewImage 必须指向主题包内存在的 PNG、JPEG 或 WebP 图片。"));
            return false;
        }
        definition.insert(QStringLiteral("previewImage"), previewImage);
    }

    if (!applyPendingResource(definition, package.resources, QStringLiteral("previewImage"),
                              QStringLiteral("previewUrl"), false, packageError) ||
        !applyPendingResource(definition, package.resources, QStringLiteral("backgroundImage"),
                              QStringLiteral("backgroundImageUrl"), false, packageError) ||
        !applyPendingResource(definition, package.resources, QStringLiteral("navigationLogo"),
                              QStringLiteral("navigationLogoUrl"), true, packageError))
    {
        setError(packageError);
        return false;
    }

    const QVariantMap navigationIcons = definition.value(QStringLiteral("icons")).toMap();
    QVariantMap navigationIconUrls;
    for (auto iterator = navigationIcons.cbegin(); iterator != navigationIcons.cend(); ++iterator)
    {
        const QString reference = iterator.value().toString();
        if (!package.archive || !isSafeResourceReference(reference, true) ||
            !package.resources.contains(reference))
        {
            setError(QStringLiteral("导航图标 %1 必须指向主题包内存在的图片或 SVG。")
                         .arg(iterator.key()));
            return false;
        }
        navigationIconUrls.insert(iterator.key(),
                                  imageDataUrl(reference, package.resources.value(reference)));
    }
    if (!navigationIconUrls.isEmpty())
    {
        definition.insert(QStringLiteral("navigationIconUrls"), navigationIconUrls);
    }
    definition.insert(QStringLiteral("packageFormat"),
                      package.archive ? QStringLiteral("archive") : QStringLiteral("json"));

    m_pendingTheme = definition;
    m_pendingThemeResources = package.resources;
    m_pendingThemeReplacesExisting = supportsTheme(importedThemeId);
    emit pendingThemeChanged();
    return true;
}

bool AppearanceSettingsViewModel::installPendingTheme()
{
    clearMessages();
    if (m_pendingTheme.isEmpty())
    {
        setError(QStringLiteral("没有等待安装的主题。"));
        return false;
    }

    const QVariantMap definition = m_pendingTheme;
    const QString importedThemeId = definition.value(QStringLiteral("themeId")).toString();

    QDir themesDirectory(m_themesDirectory);
    if (!themesDirectory.exists() && !QDir().mkpath(m_themesDirectory))
    {
        setError(QStringLiteral("无法创建主题数据目录。"));
        return false;
    }
    const QString operationId = QUuid::createUuid().toString(QUuid::Id128);
    const QString stagingName = QStringLiteral(".install-%1").arg(operationId);
    const QString backupName = QStringLiteral(".backup-%1").arg(operationId);
    const QString stagingDirectory = themesDirectory.filePath(stagingName);
    if (!themesDirectory.mkpath(stagingName))
    {
        setError(QStringLiteral("无法创建主题临时安装目录。"));
        return false;
    }

    QVariantMap storedDefinition = definition;
    storedDefinition.remove(QStringLiteral("previewUrl"));
    storedDefinition.remove(QStringLiteral("backgroundImageUrl"));
    storedDefinition.remove(QStringLiteral("navigationLogoUrl"));
    storedDefinition.remove(QStringLiteral("navigationIconUrls"));
    const QByteArray normalizedTheme = QJsonDocument(QJsonObject::fromVariantMap(storedDefinition))
                                           .toJson(QJsonDocument::Indented);
    bool writeSucceeded = writePackageFile(
        QDir(stagingDirectory).filePath(QString::fromLatin1(ThemeFileName)), normalizedTheme);
    for (auto iterator = m_pendingThemeResources.cbegin();
         writeSucceeded && iterator != m_pendingThemeResources.cend(); ++iterator)
    {
        writeSucceeded =
            writePackageFile(QDir(stagingDirectory).filePath(iterator.key()), iterator.value());
    }
    if (!writeSucceeded)
    {
        QDir(stagingDirectory).removeRecursively();
        setError(QStringLiteral("保存主题文件或资源失败。"));
        return false;
    }

    const bool replacingTheme = themesDirectory.exists(importedThemeId);
    if (replacingTheme && !themesDirectory.rename(importedThemeId, backupName))
    {
        QDir(stagingDirectory).removeRecursively();
        setError(QStringLiteral("无法准备更新已安装的主题。"));
        return false;
    }
    if (!themesDirectory.rename(stagingName, importedThemeId))
    {
        if (replacingTheme)
        {
            themesDirectory.rename(backupName, importedThemeId);
        }
        QDir(stagingDirectory).removeRecursively();
        setError(QStringLiteral("无法完成主题安装。"));
        return false;
    }
    if (replacingTheme)
    {
        QDir(themesDirectory.filePath(backupName)).removeRecursively();
    }

    const bool refreshCurrentTheme = m_themeId == importedThemeId;
    loadInstalledThemes();
    emit themesChanged();
    if (refreshCurrentTheme)
    {
        emit themeIdChanged();
    }
    else
    {
        selectTheme(importedThemeId);
    }
    setStatus(QStringLiteral("主题“%1”已安装并启用。")
                  .arg(definition.value(QStringLiteral("name")).toString()));
    cancelThemeImport();
    return true;
}

void AppearanceSettingsViewModel::cancelThemeImport()
{
    if (m_pendingTheme.isEmpty() && !m_pendingThemeReplacesExisting)
    {
        return;
    }
    m_pendingTheme.clear();
    m_pendingThemeResources.clear();
    m_pendingThemeReplacesExisting = false;
    emit pendingThemeChanged();
}

bool AppearanceSettingsViewModel::importTheme(const QUrl &fileUrl)
{
    return inspectTheme(fileUrl) && installPendingTheme();
}

bool AppearanceSettingsViewModel::exportTheme(const QString &themeId, const QUrl &targetUrl)
{
    clearMessages();
    QVariantMap definition;
    for (const QVariant &entry : m_themes)
    {
        const QVariantMap candidate = entry.toMap();
        if (candidate.value(QStringLiteral("themeId")).toString() == themeId)
        {
            definition = candidate;
            break;
        }
    }
    if (definition.isEmpty())
    {
        setError(QStringLiteral("找不到要导出的主题。"));
        return false;
    }
    if (definition.value(QStringLiteral("builtIn")).toBool())
    {
        setError(QStringLiteral("内置主题不能直接导出，请先复制为新的第三方主题。"));
        return false;
    }
    if (!targetUrl.isLocalFile())
    {
        setError(QStringLiteral("主题只能导出到本机文件。"));
        return false;
    }

    QString targetPath = targetUrl.toLocalFile();
    if (!targetPath.endsWith(QStringLiteral(".stukit-theme"), Qt::CaseInsensitive))
    {
        targetPath += QStringLiteral(".stukit-theme");
    }
    targetPath = QFileInfo(targetPath).absoluteFilePath();
    const QString relativeTarget = QDir(m_themesDirectory).relativeFilePath(targetPath);
    if (relativeTarget != QStringLiteral("..") && !relativeTarget.startsWith(QStringLiteral("../")))
    {
        setError(QStringLiteral("不能把导出文件写入应用的主题数据目录。"));
        return false;
    }

    QString exportError;
    const QString sourceDirectory = QDir(m_themesDirectory).filePath(themeId);
    if (!ThemePackageWriter::write(sourceDirectory, targetPath, exportError))
    {
        setError(exportError);
        return false;
    }
    setStatus(QStringLiteral("主题“%1”已导出到 %2。")
                  .arg(definition.value(QStringLiteral("name")).toString(), targetPath));
    return true;
}

QVariantList AppearanceSettingsViewModel::themeDiagnostics(const QString &themeId) const
{
    QVariantMap definition;
    for (const QVariant &entry : m_themes)
    {
        const QVariantMap candidate = entry.toMap();
        if (candidate.value(QStringLiteral("themeId")).toString() == themeId)
        {
            definition = candidate;
            break;
        }
    }
    if (definition.isEmpty())
    {
        return {diagnostic(QStringLiteral("主题存在性"), false, QStringLiteral("找不到该主题。"),
                           QStringLiteral("error"))};
    }

    QVariantList results;
    const auto addContrastCheck =
        [&results, &definition](const QString &label, const QString &foregroundKey,
                                const QString &backgroundKey, double minimumRatio)
    {
        const double ratio = contrastRatio(definition.value(foregroundKey).toString(),
                                           definition.value(backgroundKey).toString());
        results.push_back(diagnostic(label, ratio >= minimumRatio,
                                     QStringLiteral("对比度 %1:1，建议至少 %2:1。")
                                         .arg(ratio, 0, 'f', 2)
                                         .arg(minimumRatio, 0, 'f', 1)));
    };
    addContrastCheck(QStringLiteral("页面正文"), QStringLiteral("textPrimaryColor"),
                     QStringLiteral("backgroundColor"), 4.5);
    addContrastCheck(QStringLiteral("卡片正文"), QStringLiteral("textPrimaryColor"),
                     QStringLiteral("surfaceColor"), 4.5);
    addContrastCheck(QStringLiteral("主要按钮文字"), QStringLiteral("textOnPrimaryColor"),
                     QStringLiteral("primaryColor"), 4.5);
    addContrastCheck(QStringLiteral("次要文字"), QStringLiteral("textSecondaryColor"),
                     QStringLiteral("surfaceColor"), 4.5);
    addContrastCheck(QStringLiteral("错误状态"), QStringLiteral("errorColor"),
                     QStringLiteral("surfaceColor"), 3.0);
    addContrastCheck(QStringLiteral("成功状态"), QStringLiteral("successColor"),
                     QStringLiteral("surfaceColor"), 3.0);

    const auto addResourceCheck = [&results, &definition](const QString &label,
                                                          const QString &referenceKey,
                                                          const QString &runtimeKey)
    {
        const QString reference = definition.value(referenceKey).toString();
        if (reference.isEmpty())
        {
            return;
        }
        const QUrl resolved = definition.value(runtimeKey).toUrl();
        results.push_back(diagnostic(label, resolved.isValid() && !resolved.isEmpty(),
                                     resolved.isValid() && !resolved.isEmpty()
                                         ? QStringLiteral("资源已解析：%1").arg(reference)
                                         : QStringLiteral("资源缺失或无法读取：%1").arg(reference),
                                     QStringLiteral("error")));
    };
    addResourceCheck(QStringLiteral("主题预览图"), QStringLiteral("previewImage"),
                     QStringLiteral("previewUrl"));
    addResourceCheck(QStringLiteral("装饰背景"), QStringLiteral("backgroundImage"),
                     QStringLiteral("backgroundImageUrl"));
    addResourceCheck(QStringLiteral("导航品牌图"), QStringLiteral("navigationLogo"),
                     QStringLiteral("navigationLogoUrl"));

    const QVariantMap icons = definition.value(QStringLiteral("icons")).toMap();
    const QVariantMap resolvedIcons =
        definition.value(QStringLiteral("navigationIconUrls")).toMap();
    for (auto iterator = icons.cbegin(); iterator != icons.cend(); ++iterator)
    {
        const QUrl resolved = resolvedIcons.value(iterator.key()).toUrl();
        results.push_back(diagnostic(
            QStringLiteral("导航图标：%1").arg(iterator.key()),
            resolved.isValid() && !resolved.isEmpty(),
            resolved.isValid() && !resolved.isEmpty()
                ? QStringLiteral("资源已解析：%1").arg(iterator.value().toString())
                : QStringLiteral("资源缺失或无法读取：%1").arg(iterator.value().toString()),
            QStringLiteral("error")));
    }

    const bool hasResources =
        !definition.value(QStringLiteral("previewImage")).toString().isEmpty() ||
        !definition.value(QStringLiteral("backgroundImage")).toString().isEmpty() ||
        !definition.value(QStringLiteral("navigationLogo")).toString().isEmpty() ||
        !icons.isEmpty();
    results.push_back(diagnostic(
        QStringLiteral("主题元数据"),
        !definition.value(QStringLiteral("name")).toString().isEmpty() &&
            !definition.value(QStringLiteral("author")).toString().isEmpty() &&
            !definition.value(QStringLiteral("version")).toString().isEmpty(),
        QStringLiteral("作者 %1，版本 %2，%3。")
            .arg(definition.value(QStringLiteral("author")).toString(),
                 definition.value(QStringLiteral("version")).toString(),
                 hasResources ? QStringLiteral("包含资源") : QStringLiteral("仅设计令牌")),
        QStringLiteral("error")));
    return results;
}

bool AppearanceSettingsViewModel::removeTheme(const QString &themeId)
{
    clearMessages();
    QVariantMap definition;
    for (const QVariant &entry : m_themes)
    {
        const QVariantMap candidate = entry.toMap();
        if (candidate.value(QStringLiteral("themeId")).toString() == themeId)
        {
            definition = candidate;
            break;
        }
    }
    if (definition.isEmpty())
    {
        setError(QStringLiteral("找不到要删除的主题。"));
        return false;
    }
    if (definition.value(QStringLiteral("builtIn")).toBool())
    {
        setError(QStringLiteral("内置主题不能删除。"));
        return false;
    }

    const QString baseDirectory = QDir(m_themesDirectory).absolutePath();
    const QString themeDirectory = QDir(m_themesDirectory).absoluteFilePath(themeId);
    if (QDir(baseDirectory).relativeFilePath(themeDirectory) != themeId)
    {
        setError(QStringLiteral("主题目录不安全，已取消删除。"));
        return false;
    }

    if (!QDir(themeDirectory).removeRecursively())
    {
        setError(QStringLiteral("无法删除主题文件和资源。"));
        return false;
    }

    const bool removedCurrentTheme = m_themeId == themeId;
    loadInstalledThemes();
    emit themesChanged();
    if (removedCurrentTheme)
    {
        m_themeId = QString::fromLatin1(DefaultThemeId);
        m_settings.setValue(QString::fromLatin1(ThemeIdKey), m_themeId);
        emit themeIdChanged();
    }
    setStatus(QStringLiteral("主题“%1”已删除。")
                  .arg(definition.value(QStringLiteral("name")).toString()));
    return true;
}

void AppearanceSettingsViewModel::clearMessages()
{
    if (m_errorMessage.isEmpty() && m_statusMessage.isEmpty())
    {
        return;
    }
    m_errorMessage.clear();
    m_statusMessage.clear();
    emit messagesChanged();
}

bool AppearanceSettingsViewModel::supportsTheme(const QString &themeId) const
{
    for (const QVariant &entry : m_themes)
    {
        if (entry.toMap().value(QStringLiteral("themeId")).toString() == themeId)
        {
            return true;
        }
    }
    return false;
}

bool AppearanceSettingsViewModel::validateTheme(QVariantMap &definition,
                                                QString &errorMessage) const
{
    if (definition.value(QStringLiteral("schemaVersion")).toInt() != 1)
    {
        errorMessage = QStringLiteral("仅支持 schemaVersion 为 1 的主题文件。");
        return false;
    }

    const QString themeId = definition.value(QStringLiteral("themeId")).toString();
    static const QRegularExpression idPattern(QStringLiteral("^[a-z0-9][a-z0-9._-]{1,63}$"));
    if (!idPattern.match(themeId).hasMatch())
    {
        errorMessage =
            QStringLiteral("themeId 只能包含小写字母、数字、点、下划线和连字符，长度为 2～64。");
        return false;
    }

    const struct
    {
        const char *key;
        int maximumLength;
        const char *label;
    } textFields[] = {
        {"name", 50, "主题名称"},
        {"description", 300, "主题说明"},
        {"author", 80, "作者"},
        {"version", 30, "版本"},
    };
    for (const auto &field : textFields)
    {
        const QString value = definition.value(QString::fromLatin1(field.key)).toString().trimmed();
        if (value.isEmpty() || value.size() > field.maximumLength)
        {
            errorMessage = QStringLiteral("%1不能为空且长度不能超过 %2 个字符。")
                               .arg(QString::fromUtf8(field.label))
                               .arg(field.maximumLength);
            return false;
        }
        definition.insert(QString::fromLatin1(field.key), value);
    }

    if (!definition.contains(QStringLiteral("dark")) ||
        definition.value(QStringLiteral("dark")).metaType().id() != QMetaType::Bool)
    {
        errorMessage = QStringLiteral("主题缺少 dark 字段。");
        return false;
    }
    definition.insert(QStringLiteral("dark"), definition.value(QStringLiteral("dark")).toBool());

    for (const QString &tokenName : ColorTokenNames)
    {
        const QString value = definition.value(tokenName).toString();
        if (!isValidColor(value))
        {
            errorMessage =
                QStringLiteral("颜色令牌 %1 必须使用 #RRGGBB 或 #RRGGBBAA 格式。").arg(tokenName);
            return false;
        }
        definition.insert(tokenName, value.toUpper());
    }

    const QStringList numericTokenNames{
        QStringLiteral("radiusSmall"),
        QStringLiteral("radiusMedium"),
        QStringLiteral("radiusLarge"),
        QStringLiteral("navigationRailWidth"),
    };
    for (const QString &tokenName : numericTokenNames)
    {
        if (!definition.contains(tokenName))
        {
            errorMessage = QStringLiteral("主题缺少尺寸令牌 %1。").arg(tokenName);
            return false;
        }
    }

    const int radiusSmall = definition.value(QStringLiteral("radiusSmall")).toInt();
    const int radiusMedium = definition.value(QStringLiteral("radiusMedium")).toInt();
    const int radiusLarge = definition.value(QStringLiteral("radiusLarge")).toInt();
    if (radiusSmall < 0 || radiusSmall > radiusMedium || radiusMedium > radiusLarge ||
        radiusLarge > 32)
    {
        errorMessage = QStringLiteral("圆角必须满足 0 ≤ small ≤ medium ≤ large ≤ 32。");
        return false;
    }
    const int navigationWidth = definition.value(QStringLiteral("navigationRailWidth")).toInt();
    if (navigationWidth < 180 || navigationWidth > 320)
    {
        errorMessage = QStringLiteral("navigationRailWidth 必须在 180～320 之间。");
        return false;
    }

    const auto validateResourceField =
        [&definition, &errorMessage](const QString &key, bool allowSvg)
    {
        const QString reference = definition.value(key).toString().trimmed();
        if (reference.isEmpty())
        {
            definition.remove(key);
            return true;
        }
        if (!isSafeResourceReference(reference, allowSvg))
        {
            errorMessage = QStringLiteral("%1 必须是主题包内安全的相对图片路径。").arg(key);
            return false;
        }
        definition.insert(key, reference);
        return true;
    };
    if (!validateResourceField(QStringLiteral("previewImage"), false) ||
        !validateResourceField(QStringLiteral("backgroundImage"), false) ||
        !validateResourceField(QStringLiteral("navigationLogo"), true))
    {
        return false;
    }

    bool opacityValid = false;
    const double backgroundOpacity =
        definition.value(QStringLiteral("backgroundImageOpacity"), 0.08).toDouble(&opacityValid);
    if (!opacityValid || backgroundOpacity < 0.0 || backgroundOpacity > 0.18)
    {
        errorMessage = QStringLiteral("backgroundImageOpacity 必须在 0～0.18 之间。");
        return false;
    }
    const QString fillMode =
        definition.value(QStringLiteral("backgroundImageFillMode"), QStringLiteral("cover"))
            .toString();
    if (fillMode != QStringLiteral("cover") && fillMode != QStringLiteral("contain") &&
        fillMode != QStringLiteral("tile"))
    {
        errorMessage = QStringLiteral("backgroundImageFillMode 只支持 cover、contain 或 tile。");
        return false;
    }

    const QVariant iconsValue = definition.value(QStringLiteral("icons"));
    QVariantMap icons;
    if (iconsValue.isValid() && !iconsValue.isNull())
    {
        if (iconsValue.metaType().id() != QMetaType::QVariantMap)
        {
            errorMessage = QStringLiteral("icons 必须是导航槽位到图片路径的对象。");
            return false;
        }
        icons = iconsValue.toMap();
        for (auto iterator = icons.begin(); iterator != icons.end(); ++iterator)
        {
            const QString reference = iterator.value().toString().trimmed();
            if (!NavigationIconSlots.contains(iterator.key()) ||
                !isSafeResourceReference(reference, true))
            {
                errorMessage = QStringLiteral("导航图标槽位或资源路径无效：%1").arg(iterator.key());
                return false;
            }
            iterator.value() = reference;
        }
    }

    definition.insert(QStringLiteral("radiusSmall"), radiusSmall);
    definition.insert(QStringLiteral("radiusMedium"), radiusMedium);
    definition.insert(QStringLiteral("radiusLarge"), radiusLarge);
    definition.insert(QStringLiteral("navigationRailWidth"), navigationWidth);
    definition.insert(QStringLiteral("backgroundImageOpacity"), backgroundOpacity);
    definition.insert(QStringLiteral("backgroundImageFillMode"), fillMode);
    if (icons.isEmpty())
    {
        definition.remove(QStringLiteral("icons"));
    }
    else
    {
        definition.insert(QStringLiteral("icons"), icons);
    }
    definition.insert(QStringLiteral("schemaVersion"), 1);
    definition.insert(QStringLiteral("builtIn"), false);
    return true;
}

void AppearanceSettingsViewModel::loadInstalledThemes()
{
    m_themes = builtInThemes();
    QDir themesDirectory(m_themesDirectory);
    if (!themesDirectory.exists())
    {
        return;
    }

    const QStringList directories =
        themesDirectory.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &directoryName : directories)
    {
        QFile file(themesDirectory.filePath(directoryName + QLatin1Char('/') +
                                            QString::fromLatin1(ThemeFileName)));
        if (!file.open(QIODevice::ReadOnly) || file.size() <= 0 ||
            file.size() > MaximumThemeFileSize)
        {
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            continue;
        }

        QVariantMap definition = document.object().toVariantMap();
        QString validationError;
        if (!validateTheme(definition, validationError) ||
            definition.value(QStringLiteral("themeId")).toString() != directoryName ||
            supportsTheme(directoryName))
        {
            continue;
        }
        applyInstalledResources(definition, themesDirectory.filePath(directoryName));
        m_themes.push_back(definition);
    }
}

void AppearanceSettingsViewModel::setError(QString message)
{
    m_errorMessage = std::move(message);
    m_statusMessage.clear();
    emit messagesChanged();
}

void AppearanceSettingsViewModel::setStatus(QString message)
{
    m_statusMessage = std::move(message);
    m_errorMessage.clear();
    emit messagesChanged();
}

} // namespace stukit::app
