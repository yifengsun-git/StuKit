#include "GitHubReleaseUpdateService.h"

#include "GitHubReleaseParser.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>

#include <memory>
#include <utility>

namespace stukit::update {
namespace {

const QUrl defaultLatestReleaseUrl(
    QStringLiteral("https://api.github.com/repos/yifengsun-git/StuKit/releases/latest"));

UpdateFailure networkFailure(const QNetworkReply &reply)
{
    return UpdateFailure{
        .error = UpdateServiceError::NetworkUnavailable,
        .detail = reply.errorString(),
    };
}

UpdateFailure storageFailure(QString detail)
{
    return UpdateFailure{
        .error = UpdateServiceError::StorageUnavailable,
        .detail = std::move(detail),
    };
}

struct DownloadContext
{
    explicit DownloadContext(const QString &destination)
        : file(destination)
        , hash(QCryptographicHash::Sha256)
    {
    }

    QSaveFile file;
    QCryptographicHash hash;
    QString writeError;
};

} // namespace

GitHubReleaseUpdateService::GitHubReleaseUpdateService(QObject *parent)
    : GitHubReleaseUpdateService(defaultLatestReleaseUrl, parent)
{
}

GitHubReleaseUpdateService::GitHubReleaseUpdateService(
    QUrl latestReleaseUrl,
    QObject *parent)
    : QObject(parent)
    , m_latestReleaseUrl(std::move(latestReleaseUrl))
{
}

GitHubReleaseUpdateService::~GitHubReleaseUpdateService()
{
    cancel();
}

void GitHubReleaseUpdateService::checkLatest(UpdateCheckCallback callback)
{
    cancel();
    const quint64 generation = m_operationGeneration;
    QNetworkReply *reply = m_network.get(requestFor(m_latestReleaseUrl));
    m_activeReply = reply;
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, generation,
             callback = std::move(callback)]() mutable {
        const bool wasCancelled = generation != m_operationGeneration;
        if (m_activeReply == reply) {
            m_activeReply.clear();
        }
        if (wasCancelled) {
            callback(UpdateFailure{
                .error = UpdateServiceError::Cancelled,
                .detail = QStringLiteral("更新检查已取消"),
            });
        } else if (reply->error() != QNetworkReply::NoError) {
            callback(networkFailure(*reply));
        } else {
            callback(GitHubReleaseParser::parse(reply->readAll()));
        }
        reply->deleteLater();
    });
}

void GitHubReleaseUpdateService::download(
    const UpdateInfo &update,
    const QString &destinationDirectory,
    UpdateProgressCallback progress,
    UpdateDownloadCallback callback)
{
    cancel();
    const quint64 generation = m_operationGeneration;
    if (!update.hasWindowsPackage()) {
        callback(storageFailure(QStringLiteral("当前 Release 没有 Windows 安装包")));
        return;
    }
    if (update.sha256.isEmpty()) {
        callback(UpdateFailure{
            .error = UpdateServiceError::IntegrityCheckFailed,
            .detail = QStringLiteral("GitHub Release 没有提供安装包 SHA-256"),
        });
        return;
    }
    if (!QDir().mkpath(destinationDirectory)) {
        callback(storageFailure(QStringLiteral("无法创建更新缓存目录")));
        return;
    }

    const QString destination = QDir(destinationDirectory).filePath(
        update.packageFileName);
    const auto download = std::make_shared<DownloadContext>(destination);
    if (!download->file.open(QIODevice::WriteOnly)) {
        const QString detail = download->file.errorString();
        callback(storageFailure(detail));
        return;
    }

    QNetworkReply *reply = m_network.get(requestFor(update.packageUrl));
    m_activeReply = reply;
    connect(reply, &QNetworkReply::readyRead, this, [download, reply]() {
        const QByteArray data = reply->readAll();
        if (data.isEmpty()) {
            return;
        }
        download->hash.addData(data);
        if (download->file.write(data) != data.size()) {
            download->writeError = download->file.errorString();
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::downloadProgress, this,
            [progress](qint64 received, qint64 total) {
        progress(received, total);
    });
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, generation, destination, download,
             expectedHash = update.sha256,
             callback = std::move(callback)]() mutable {
        const bool wasCancelled = generation != m_operationGeneration;
        if (m_activeReply == reply) {
            m_activeReply.clear();
        }
        if (wasCancelled) {
            download->file.cancelWriting();
            callback(UpdateFailure{
                .error = UpdateServiceError::Cancelled,
                .detail = QStringLiteral("更新下载已取消"),
            });
        } else if (!download->writeError.isEmpty()) {
            const UpdateFailure failure = storageFailure(
                download->writeError);
            download->file.cancelWriting();
            callback(failure);
        } else if (reply->error() != QNetworkReply::NoError) {
            const UpdateFailure failure = networkFailure(*reply);
            download->file.cancelWriting();
            callback(failure);
        } else {
            const QByteArray actualHash = download->hash.result().toHex();
            if (!expectedHash.isEmpty() && actualHash != expectedHash) {
                download->file.cancelWriting();
                callback(UpdateFailure{
                    .error = UpdateServiceError::IntegrityCheckFailed,
                    .detail = QStringLiteral("安装包 SHA-256 校验失败"),
                });
            } else if (!download->file.commit()) {
                const UpdateFailure failure = storageFailure(
                    download->file.errorString());
                download->file.cancelWriting();
                callback(failure);
            } else {
                callback(destination);
            }
        }
        reply->deleteLater();
    });
}

void GitHubReleaseUpdateService::cancel()
{
    ++m_operationGeneration;
    if (m_activeReply != nullptr) {
        m_activeReply->abort();
    }
}

QNetworkRequest GitHubReleaseUpdateService::requestFor(const QUrl &url) const
{
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    request.setRawHeader("User-Agent", "StuKit-Update-Client");
    request.setAttribute(
        QNetworkRequest::RedirectPolicyAttribute,
        QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(30'000);
    return request;
}

} // namespace stukit::update
