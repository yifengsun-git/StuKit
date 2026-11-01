#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class QSettings;
class QUrl;

namespace stukit::app
{

class AppearanceSettingsViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString themeId READ themeId WRITE setThemeId NOTIFY themeIdChanged)
    Q_PROPERTY(QVariantList themes READ themes NOTIFY themesChanged)
    Q_PROPERTY(QVariantMap currentTheme READ currentTheme NOTIFY themeIdChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY messagesChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY messagesChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY messagesChanged)
    Q_PROPERTY(bool hasPendingTheme READ hasPendingTheme NOTIFY pendingThemeChanged)
    Q_PROPERTY(QVariantMap pendingTheme READ pendingTheme NOTIFY pendingThemeChanged)
    Q_PROPERTY(bool pendingThemeReplacesExisting READ pendingThemeReplacesExisting NOTIFY
                   pendingThemeChanged)

public:
    explicit AppearanceSettingsViewModel(QSettings &settings, QString themesDirectory,
                                         QObject *parent = nullptr);

    [[nodiscard]] QString themeId() const;
    [[nodiscard]] QVariantList themes() const;
    [[nodiscard]] QVariantMap currentTheme() const;
    [[nodiscard]] bool hasError() const;
    [[nodiscard]] QString errorMessage() const;
    [[nodiscard]] QString statusMessage() const;
    [[nodiscard]] bool hasPendingTheme() const;
    [[nodiscard]] QVariantMap pendingTheme() const;
    [[nodiscard]] bool pendingThemeReplacesExisting() const;

    void setThemeId(const QString &themeId);
    Q_INVOKABLE bool selectTheme(const QString &themeId);
    Q_INVOKABLE bool inspectTheme(const QUrl &fileUrl);
    Q_INVOKABLE bool installPendingTheme();
    Q_INVOKABLE void cancelThemeImport();
    Q_INVOKABLE bool importTheme(const QUrl &fileUrl);
    Q_INVOKABLE bool exportTheme(const QString &themeId, const QUrl &targetUrl);
    Q_INVOKABLE QVariantList themeDiagnostics(const QString &themeId) const;
    Q_INVOKABLE bool removeTheme(const QString &themeId);
    Q_INVOKABLE void clearMessages();

signals:
    void themeIdChanged();
    void themesChanged();
    void messagesChanged();
    void pendingThemeChanged();

private:
    [[nodiscard]] bool supportsTheme(const QString &themeId) const;
    [[nodiscard]] bool validateTheme(QVariantMap &definition, QString &errorMessage) const;
    void loadInstalledThemes();
    void setError(QString message);
    void setStatus(QString message);

    QSettings &m_settings;
    QString m_themesDirectory;
    QVariantList m_themes;
    QString m_themeId;
    QString m_errorMessage;
    QString m_statusMessage;
    QVariantMap m_pendingTheme;
    QMap<QString, QByteArray> m_pendingThemeResources;
    bool m_pendingThemeReplacesExisting{false};
};

} // namespace stukit::app
