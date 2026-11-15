#include "InMemoryPlanRevisionRepository.h"

#include <algorithm>

namespace stukit::planning {

bool InMemoryPlanRevisionRepository::save(
    const PlanRevision &revision, QString &error)
{
    error.clear();
    const auto duplicate = std::find_if(
        m_revisions.cbegin(), m_revisions.cend(),
        [&revision](const PlanRevision &candidate) {
            return candidate.projectId() == revision.projectId()
                && candidate.revisionNumber() == revision.revisionNumber();
        });
    if (duplicate != m_revisions.cend()) {
        error = QStringLiteral("规划版本号已经存在");
        return false;
    }
    m_revisions.push_back(revision);
    return true;
}

std::vector<PlanRevision> InMemoryPlanRevisionRepository::forProject(
    const QUuid &projectId, QString &error) const
{
    error.clear();
    std::vector<PlanRevision> result;
    for (const auto &revision : m_revisions) {
        if (revision.projectId() == projectId) {
            result.push_back(revision);
        }
    }
    std::sort(result.begin(), result.end(),
        [](const PlanRevision &left, const PlanRevision &right) {
            return left.revisionNumber() > right.revisionNumber();
        });
    return result;
}

std::optional<PlanRevision> InMemoryPlanRevisionRepository::latest(
    const QUuid &projectId, QString &error) const
{
    const auto revisions = forProject(projectId, error);
    return revisions.empty()
        ? std::nullopt : std::optional<PlanRevision>(revisions.front());
}

} // namespace stukit::planning
