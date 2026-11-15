#pragma once

#include <QDateTime>
#include <QString>
#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::planning {

struct PlanRevisionData
{
    QUuid id;
    QUuid projectId;
    int revisionNumber{0};
    QString markdownContent;
    QString providerId;
    QString modelId;
    std::vector<QUuid> sourceIds;
    QDateTime createdAtUtc;
};

class PlanRevision final
{
public:
    static constexpr qsizetype MaximumMarkdownLength = 2 * 1024 * 1024;
    static constexpr qsizetype MaximumProviderIdLength = 80;
    static constexpr qsizetype MaximumModelIdLength = 160;

    static std::optional<PlanRevision> create(PlanRevisionData data);

    [[nodiscard]] const QUuid &id() const noexcept;
    [[nodiscard]] const QUuid &projectId() const noexcept;
    [[nodiscard]] int revisionNumber() const noexcept;
    [[nodiscard]] const QString &markdownContent() const noexcept;
    [[nodiscard]] const QString &providerId() const noexcept;
    [[nodiscard]] const QString &modelId() const noexcept;
    [[nodiscard]] const std::vector<QUuid> &sourceIds() const noexcept;
    [[nodiscard]] const QDateTime &createdAtUtc() const noexcept;
    [[nodiscard]] PlanRevisionData data() const;

private:
    explicit PlanRevision(PlanRevisionData data);

    PlanRevisionData m_data;
};

} // namespace stukit::planning
