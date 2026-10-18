#include "GitHubReleaseParser.h"

#include "UpdateVersion.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QVersionNumber>

namespace stukit::update {
namespace {

UpdateFailure invalidResponse(const QString &detail)
{
    return UpdateFailure{
        .error = UpdateServiceError::InvalidResponse,
        .detail = detail,
    };
}

QByteArray readSha256(const QJsonObject &asset)
{
    QString digest = asset.value(QStringLiteral("digest")).toString().trimmed();
    if (digest.startsWith(QStringLiteral("sha256:"), Qt::CaseInsensitive)) {
        digest.remove(0, 7);
    }
    static const QRegularExpression sha256Pattern(
        QStringLiteral("^[0-9a-fA-F]{64}$"));
    return sha256Pattern.match(digest).hasMatch()
        ? digest.toLatin1().toLower()
        : QByteArray{};
}

} // namespace

UpdateCheckResult GitHubReleaseParser::parse(const QByteArray &json)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return invalidResponse(QStringLiteral("GitHub 返回的数据不是有效的 JSON"));
    }

    const QJsonObject release = document.object();
    const QString tagName = release.value(QStringLiteral("tag_name"))
                                .toString().trimmed();
    const QString version = UpdateVersion::normalized(tagName);
    if (version.isEmpty()
        || QVersionNumber::fromString(version).isNull()) {
        return invalidResponse(QStringLiteral("GitHub Release 的版本标签无效"));
    }

    UpdateInfo result{
        .version = version,
        .tagName = tagName,
        .releaseNotes = release.value(QStringLiteral("body")).toString(),
        .releasePageUrl = QUrl(
            release.value(QStringLiteral("html_url")).toString()),
        .publishedAt = QDateTime::fromString(
            release.value(QStringLiteral("published_at")).toString(),
            Qt::ISODate),
    };

    static const QRegularExpression installerPattern(
        QStringLiteral("^StuKit-.+-windows-x64-setup\\.exe$"),
        QRegularExpression::CaseInsensitiveOption);
    const QJsonArray assets = release.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue &assetValue : assets) {
        const QJsonObject asset = assetValue.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        if (!installerPattern.match(name).hasMatch()) {
            continue;
        }
        result.packageFileName = name;
        result.packageUrl = QUrl(
            asset.value(QStringLiteral("browser_download_url")).toString());
        result.packageSize = asset.value(QStringLiteral("size")).toInteger();
        result.sha256 = readSha256(asset);
        break;
    }
    return result;
}

} // namespace stukit::update
