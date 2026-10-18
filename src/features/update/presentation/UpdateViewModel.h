#pragma once

#include "IUpdateInstaller.h"
#include "IUpdatePreparation.h"
#include "IUpdateService.h"

#include <QObject>
#include <QString>

namespace stukit::update {

class UpdateViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool supported READ supported CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool canDownload READ canDownload NOTIFY stateChanged)
    Q_PROPERTY(bool readyToInstall READ readyToInstall NOTIFY stateChanged)
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY stateChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY stateChanged)
    Q_PROPERTY(QString publishedAtText READ publishedAtText NOTIFY stateChanged)
    Q_PROPERTY(QString packageSizeText READ packageSizeText NOTIFY stateChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(double downloadProgress READ downloadProgress NOTIFY stateChanged)

public:
    UpdateViewModel(
        IUpdateService &service,
        IUpdateInstaller &installer,
        IUpdatePreparation &preparation,
        QString currentVersion,
        QObject *parent = nullptr);
    ~UpdateViewModel() override;

    [[nodiscard]] bool supported() const noexcept;
    [[nodiscard]] bool busy() const noexcept;
    [[nodiscard]] bool updateAvailable() const noexcept;
    [[nodiscard]] bool canDownload() const noexcept;
    [[nodiscard]] bool readyToInstall() const noexcept;
    [[nodiscard]] const QString &currentVersion() const noexcept;
    [[nodiscard]] const QString &latestVersion() const noexcept;
    [[nodiscard]] const QString &releaseNotes() const noexcept;
    [[nodiscard]] QString publishedAtText() const;
    [[nodiscard]] QString packageSizeText() const;
    [[nodiscard]] const QString &statusMessage() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;
    [[nodiscard]] double downloadProgress() const noexcept;

    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void downloadUpdate();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void installUpdate();
    Q_INVOKABLE void openReleasePage();

signals:
    void stateChanged();

private:
    enum class State
    {
        Idle,
        Checking,
        UpToDate,
        UpdateAvailable,
        Downloading,
        ReadyToInstall,
        Installing,
        Error
    };

    void setError(const UpdateFailure &failure);
    void resetDownload();

    IUpdateService &m_service;
    IUpdateInstaller &m_installer;
    IUpdatePreparation &m_preparation;
    QString m_currentVersion;
    UpdateInfo m_update;
    State m_state{State::Idle};
    QString m_statusMessage{QStringLiteral("点击“检查更新”获取最新版本")};
    QString m_errorMessage;
    QString m_downloadedPackagePath;
    double m_downloadProgress{0.0};
};

} // namespace stukit::update
