#include "UpdateViewModel.h"

#include "UpdateVersion.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QLocale>
#include <QPointer>
#include <QStandardPaths>
#include <QTimer>

#include <utility>

namespace stukit::update {

UpdateViewModel::UpdateViewModel(
    IUpdateService &service,
    IUpdateInstaller &installer,
    IUpdatePreparation &preparation,
    QString currentVersion,
    QObject *parent)
    : QObject(parent)
    , m_service(service)
    , m_installer(installer)
    , m_preparation(preparation)
    , m_currentVersion(std::move(currentVersion))
{
}

UpdateViewModel::~UpdateViewModel()
{
    m_service.cancel();
}

bool UpdateViewModel::supported() const noexcept
{
    return m_installer.supported();
}

bool UpdateViewModel::busy() const noexcept
{
    return m_state == State::Checking || m_state == State::Downloading
        || m_state == State::Installing;
}

bool UpdateViewModel::updateAvailable() const noexcept
{
    return m_state == State::UpdateAvailable
        || m_state == State::Downloading
        || m_state == State::ReadyToInstall;
}

bool UpdateViewModel::canDownload() const noexcept
{
    return m_state == State::UpdateAvailable
        && m_update.hasWindowsPackage()
        && !m_update.sha256.isEmpty();
}

bool UpdateViewModel::readyToInstall() const noexcept
{
    return m_state == State::ReadyToInstall
        && !m_downloadedPackagePath.isEmpty();
}

const QString &UpdateViewModel::currentVersion() const noexcept
{
    return m_currentVersion;
}

const QString &UpdateViewModel::latestVersion() const noexcept
{
    return m_update.version;
}

const QString &UpdateViewModel::releaseNotes() const noexcept
{
    return m_update.releaseNotes;
}

QString UpdateViewModel::publishedAtText() const
{
    if (!m_update.publishedAt.isValid()) {
        return {};
    }
    return QLocale(QLocale::Chinese).toString(
        m_update.publishedAt.toLocalTime(),
        QStringLiteral("yyyy-MM-dd HH:mm"));
}

QString UpdateViewModel::packageSizeText() const
{
    if (m_update.packageSize <= 0) {
        return {};
    }
    constexpr double bytesPerMegabyte = 1024.0 * 1024.0;
    return QStringLiteral("%1 MB").arg(
        QLocale::c().toString(m_update.packageSize / bytesPerMegabyte, 'f', 1));
}

const QString &UpdateViewModel::statusMessage() const noexcept
{
    return m_statusMessage;
}

const QString &UpdateViewModel::errorMessage() const noexcept
{
    return m_errorMessage;
}

double UpdateViewModel::downloadProgress() const noexcept
{
    return m_downloadProgress;
}

void UpdateViewModel::checkForUpdates()
{
    if (!supported() || busy()) {
        return;
    }
    resetDownload();
    m_update = {};
    m_state = State::Checking;
    m_statusMessage = QStringLiteral("正在连接 GitHub 检查更新…");
    m_errorMessage.clear();
    emit stateChanged();

    const QPointer<UpdateViewModel> self(this);
    m_service.checkLatest([self](UpdateCheckResult result) {
        if (self == nullptr) {
            return;
        }
        if (std::holds_alternative<UpdateFailure>(result)) {
            const UpdateFailure failure = std::get<UpdateFailure>(
                std::move(result));
            if (failure.error != UpdateServiceError::Cancelled) {
                self->setError(failure);
            }
            return;
        }

        self->m_update = std::get<UpdateInfo>(std::move(result));
        self->m_errorMessage.clear();
        if (!UpdateVersion::isNewer(
                self->m_update.version, self->m_currentVersion)) {
            self->m_state = State::UpToDate;
            self->m_statusMessage = QStringLiteral("当前已经是最新版本");
        } else {
            self->m_state = State::UpdateAvailable;
            if (!self->m_update.hasWindowsPackage()) {
                self->m_statusMessage = QStringLiteral(
                    "发现新版本，但该 Release 缺少 Windows 安装包");
            } else if (self->m_update.sha256.isEmpty()) {
                self->m_statusMessage = QStringLiteral(
                    "发现新版本，但安装包缺少 SHA-256，已禁止下载");
            } else {
                self->m_statusMessage = QStringLiteral(
                    "发现新版本，可以下载安装");
            }
        }
        emit self->stateChanged();
    });
}

void UpdateViewModel::downloadUpdate()
{
    if (!canDownload()) {
        return;
    }
    QString cacheDirectory = QStandardPaths::writableLocation(
        QStandardPaths::CacheLocation);
    if (cacheDirectory.isEmpty()) {
        cacheDirectory = QDir::tempPath();
    }
    cacheDirectory = QDir(cacheDirectory).filePath(QStringLiteral("updates"));

    m_state = State::Downloading;
    m_downloadProgress = 0.0;
    m_statusMessage = QStringLiteral("正在下载安装包…");
    m_errorMessage.clear();
    emit stateChanged();

    const QPointer<UpdateViewModel> self(this);
    m_service.download(
        m_update,
        cacheDirectory,
        [self](qint64 received, qint64 total) {
            if (self == nullptr || total <= 0) {
                return;
            }
            self->m_downloadProgress = qBound(
                0.0,
                static_cast<double>(received) / static_cast<double>(total),
                1.0);
            emit self->stateChanged();
        },
        [self](UpdateDownloadResult result) {
            if (self == nullptr) {
                return;
            }
            if (std::holds_alternative<UpdateFailure>(result)) {
                const UpdateFailure failure = std::get<UpdateFailure>(
                    std::move(result));
                if (failure.error != UpdateServiceError::Cancelled) {
                    self->setError(failure);
                }
                return;
            }
            self->m_downloadedPackagePath = std::get<QString>(
                std::move(result));
            self->m_downloadProgress = 1.0;
            self->m_state = State::ReadyToInstall;
            self->m_statusMessage = QStringLiteral(
                "安装包已下载并通过校验，可以开始安装");
            self->m_errorMessage.clear();
            emit self->stateChanged();
        });
}

void UpdateViewModel::cancel()
{
    if (!busy()) {
        return;
    }
    m_service.cancel();
    m_state = m_update.version.isEmpty()
        ? State::Idle : State::UpdateAvailable;
    m_statusMessage = QStringLiteral("操作已取消");
    m_errorMessage.clear();
    emit stateChanged();
}

void UpdateViewModel::installUpdate()
{
    if (!readyToInstall()) {
        return;
    }
    QString backupPath;
    QString error;
    if (!m_preparation.prepare(m_update.version, backupPath, error)) {
        setError(UpdateFailure{
            .error = UpdateServiceError::StorageUnavailable,
            .detail = QStringLiteral("更新前的数据备份失败：%1").arg(error),
        });
        return;
    }
    if (!m_installer.startInstaller(m_downloadedPackagePath, error)) {
        setError(UpdateFailure{
            .error = UpdateServiceError::StorageUnavailable,
            .detail = std::move(error),
        });
        return;
    }
    m_state = State::Installing;
    m_statusMessage = backupPath.isEmpty()
        ? QStringLiteral("安装程序已启动，StuKit 即将退出")
        : QStringLiteral("数据已备份，安装程序已启动，StuKit 即将退出");
    emit stateChanged();
    QTimer::singleShot(250, QCoreApplication::instance(), []() {
        QCoreApplication::quit();
    });
}

void UpdateViewModel::openReleasePage()
{
    if (m_update.releasePageUrl.isValid()) {
        QDesktopServices::openUrl(m_update.releasePageUrl);
    }
}

void UpdateViewModel::setError(const UpdateFailure &failure)
{
    m_state = State::Error;
    switch (failure.error) {
    case UpdateServiceError::NetworkUnavailable:
        m_errorMessage = QStringLiteral("无法连接更新服务器，请检查网络后重试");
        break;
    case UpdateServiceError::InvalidResponse:
        m_errorMessage = QStringLiteral("无法识别 GitHub 返回的版本信息");
        break;
    case UpdateServiceError::StorageUnavailable:
        m_errorMessage = QStringLiteral("无法保存或启动安装包");
        break;
    case UpdateServiceError::IntegrityCheckFailed:
        m_errorMessage = QStringLiteral("安装包校验失败，已取消安装");
        break;
    case UpdateServiceError::Cancelled:
        m_errorMessage.clear();
        break;
    }
    if (!failure.detail.trimmed().isEmpty()) {
        m_errorMessage += QStringLiteral("：%1").arg(failure.detail);
    }
    m_statusMessage.clear();
    emit stateChanged();
}

void UpdateViewModel::resetDownload()
{
    m_downloadedPackagePath.clear();
    m_downloadProgress = 0.0;
}

} // namespace stukit::update
