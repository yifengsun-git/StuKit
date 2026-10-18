#pragma once

#include "IUpdateService.h"

#include <QNetworkAccessManager>
#include <QPointer>
#include <QUrl>

class QNetworkReply;

namespace stukit::update {

class GitHubReleaseUpdateService final : public QObject, public IUpdateService
{
    Q_OBJECT

public:
    explicit GitHubReleaseUpdateService(QObject *parent = nullptr);
    explicit GitHubReleaseUpdateService(
        QUrl latestReleaseUrl,
        QObject *parent = nullptr);
    ~GitHubReleaseUpdateService() override;

    void checkLatest(UpdateCheckCallback callback) override;
    void download(
        const UpdateInfo &update,
        const QString &destinationDirectory,
        UpdateProgressCallback progress,
        UpdateDownloadCallback callback) override;
    void cancel() override;

private:
    [[nodiscard]] QNetworkRequest requestFor(const QUrl &url) const;

    QNetworkAccessManager m_network;
    QUrl m_latestReleaseUrl;
    QPointer<QNetworkReply> m_activeReply;
    quint64 m_operationGeneration{0};
};

} // namespace stukit::update
