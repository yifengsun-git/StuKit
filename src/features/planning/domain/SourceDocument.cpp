#include "SourceDocument.h"

#include <QCryptographicHash>

#include <utility>

namespace stukit::planning {
namespace {

bool isUtc(const QDateTime &value)
{
    return value.isValid() && value.timeSpec() == Qt::UTC;
}

bool isValidKind(SourceDocumentKind kind)
{
    switch (kind) {
    case SourceDocumentKind::Curriculum:
    case SourceDocumentKind::Opportunity:
    case SourceDocumentKind::PersonalProfile:
    case SourceDocumentKind::Other:
        return true;
    }
    return false;
}

bool isValidFormat(SourceDocumentFormat format)
{
    switch (format) {
    case SourceDocumentFormat::PlainText:
    case SourceDocumentFormat::Markdown:
    case SourceDocumentFormat::Csv:
        return true;
    }
    return false;
}

QString normalizeContent(QString content)
{
    content.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    content.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return content;
}

} // namespace

std::optional<SourceDocument> SourceDocument::create(SourceDocumentData data)
{
    data.title = data.title.trimmed();
    data.originalFileName = data.originalFileName.trimmed();
    data.content = normalizeContent(std::move(data.content));
    data.contentSha256 = data.contentSha256.trimmed().toLower();

    const QString calculatedHash = calculateContentHash(data.content);
    if (data.contentSha256.isEmpty()) {
        data.contentSha256 = calculatedHash;
    }

    if (data.id.isNull()
        || data.projectId.isNull()
        || data.title.isEmpty()
        || data.title.size() > MaximumTitleLength
        || !isValidKind(data.kind)
        || !isValidFormat(data.format)
        || data.originalFileName.size() > MaximumFileNameLength
        || data.content.trimmed().isEmpty()
        || data.content.size() > MaximumContentLength
        || data.contentSha256 != calculatedHash
        || !isUtc(data.importedAtUtc)) {
        return std::nullopt;
    }

    return SourceDocument(std::move(data));
}

QString SourceDocument::calculateContentHash(const QString &normalizedContent)
{
    return QString::fromLatin1(QCryptographicHash::hash(
        normalizedContent.toUtf8(), QCryptographicHash::Sha256).toHex());
}

SourceDocument::SourceDocument(SourceDocumentData data)
    : m_data(std::move(data))
{
}

const QUuid &SourceDocument::id() const noexcept { return m_data.id; }
const QUuid &SourceDocument::projectId() const noexcept
{
    return m_data.projectId;
}
const QString &SourceDocument::title() const noexcept { return m_data.title; }
SourceDocumentKind SourceDocument::kind() const noexcept { return m_data.kind; }
SourceDocumentFormat SourceDocument::format() const noexcept
{
    return m_data.format;
}
const QString &SourceDocument::originalFileName() const noexcept
{
    return m_data.originalFileName;
}
const QString &SourceDocument::content() const noexcept { return m_data.content; }
const QString &SourceDocument::contentSha256() const noexcept
{
    return m_data.contentSha256;
}
const QDateTime &SourceDocument::importedAtUtc() const noexcept
{
    return m_data.importedAtUtc;
}
SourceDocumentData SourceDocument::data() const { return m_data; }

} // namespace stukit::planning
