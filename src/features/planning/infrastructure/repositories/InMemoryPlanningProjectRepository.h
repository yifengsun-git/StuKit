#pragma once

#include "IPlanningProjectRepository.h"

#include <vector>

namespace stukit::planning {

class InMemoryPlanningProjectRepository final
    : public IPlanningProjectRepository
{
public:
    bool save(const PlanningProject &project, QString &error) override;
    std::vector<PlanningProject> all(QString &error) const override;
    std::optional<PlanningProject> findById(
        const QUuid &id, QString &error) const override;

private:
    std::vector<PlanningProject> m_projects;
};

} // namespace stukit::planning
