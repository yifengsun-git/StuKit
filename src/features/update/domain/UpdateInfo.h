#pragma once

#include <QDateTime>
#include <QString>
#include <QUrl>

namespace stukit::update {

struct UpdateInfo
{
    QString version;
    QString tagName;
    QString releaseNotes;
    QString packageFileName;
    QUrl releasePageUrl;
    QUrl packageUrl;
    QDateTime publishedAt;
    qint64 packageSize{0};
    QByteArray sha256;

    [[nodiscard]] bool hasWindowsPackage() const noexcept
    {
        return packageUrl.isValid() && !packageFileName.isEmpty();
    }
};

} // namespace stukit::update
