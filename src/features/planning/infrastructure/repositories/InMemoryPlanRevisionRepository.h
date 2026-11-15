#pragma once

#include "IPlanRevisionRepository.h"

namespace stukit::planning {

class InMemoryPlanRevisionRepository final
    : public IPlanRevisionRepository
{
public:
    bool save(const PlanRevision &revision, QString &error) override;
    std::vector<PlanRevision> forProject(
        const QUuid &projectId, QString &error) const override;
    std::optional<PlanRevision> latest(
        const QUuid &projectId, QString &error) const override;

private:
    std::vector<PlanRevision> m_revisions;
};

} // namespace stukit::planning
