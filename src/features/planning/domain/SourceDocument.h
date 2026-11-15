#pragma once

#include "PlanningTypes.h"

#include <QDateTime>
#include <QString>
#include <QUuid>

#include <optional>

namespace stukit::planning {

struct SourceDocumentData
{
    QUuid id;
    QUuid projectId;
    QString title;
    SourceDocumentKind kind{SourceDocumentKind::Other};
    SourceDocumentFormat format{SourceDocumentFormat::PlainText};
    QString originalFileName;
    QString content;
    QString contentSha256;
    QDateTime importedAtUtc;
};

class SourceDocument final
{
public:
    static constexpr qsizetype MaximumTitleLength = 160;
    static constexpr qsizetype MaximumFileNameLength = 255;
    static constexpr qsizetype MaximumContentLength = 2 * 1024 * 1024;

    static std::optional<SourceDocument> create(SourceDocumentData data);
    static QString calculateContentHash(const QString &normalizedContent);

    [[nodiscard]] const QUuid &id() const noexcept;
    [[nodiscard]] const QUuid &projectId() const noexcept;
    [[nodiscard]] const QString &title() const noexcept;
    [[nodiscard]] SourceDocumentKind kind() const noexcept;
    [[nodiscard]] SourceDocumentFormat format() const noexcept;
    [[nodiscard]] const QString &originalFileName() const noexcept;
    [[nodiscard]] const QString &content() const noexcept;
    [[nodiscard]] const QString &contentSha256() const noexcept;
    [[nodiscard]] const QDateTime &importedAtUtc() const noexcept;
    [[nodiscard]] SourceDocumentData data() const;

private:
    explicit SourceDocument(SourceDocumentData data);

    SourceDocumentData m_data;
};

} // namespace stukit::planning
