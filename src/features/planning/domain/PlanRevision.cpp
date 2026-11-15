#include "PlanRevision.h"

#include <QSet>

#include <utility>

namespace stukit::planning {

std::optional<PlanRevision> PlanRevision::create(PlanRevisionData data)
{
    data.providerId = data.providerId.trimmed();
    data.modelId = data.modelId.trimmed();

    QSet<QUuid> uniqueSourceIds;
    for (const QUuid &sourceId : data.sourceIds) {
        if (sourceId.isNull() || uniqueSourceIds.contains(sourceId)) {
            return std::nullopt;
        }
        uniqueSourceIds.insert(sourceId);
    }

    if (data.id.isNull()
        || data.projectId.isNull()
        || data.revisionNumber < 1
        || data.markdownContent.trimmed().isEmpty()
        || data.markdownContent.size() > MaximumMarkdownLength
        || data.providerId.size() > MaximumProviderIdLength
        || data.modelId.size() > MaximumModelIdLength
        || !data.createdAtUtc.isValid()
        || data.createdAtUtc.timeSpec() != Qt::UTC) {
        return std::nullopt;
    }

    return PlanRevision(std::move(data));
}

PlanRevision::PlanRevision(PlanRevisionData data)
    : m_data(std::move(data))
{
}

const QUuid &PlanRevision::id() const noexcept { return m_data.id; }
const QUuid &PlanRevision::projectId() const noexcept
{
    return m_data.projectId;
}
int PlanRevision::revisionNumber() const noexcept
{
    return m_data.revisionNumber;
}
const QString &PlanRevision::markdownContent() const noexcept
{
    return m_data.markdownContent;
}
const QString &PlanRevision::providerId() const noexcept
{
    return m_data.providerId;
}
const QString &PlanRevision::modelId() const noexcept
{
    return m_data.modelId;
}
const std::vector<QUuid> &PlanRevision::sourceIds() const noexcept
{
    return m_data.sourceIds;
}
const QDateTime &PlanRevision::createdAtUtc() const noexcept
{
    return m_data.createdAtUtc;
}
PlanRevisionData PlanRevision::data() const { return m_data; }

} // namespace stukit::planning
