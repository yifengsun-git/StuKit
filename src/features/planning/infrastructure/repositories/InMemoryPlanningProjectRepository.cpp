#include "InMemoryPlanningProjectRepository.h"

#include <algorithm>

namespace stukit::planning {

bool InMemoryPlanningProjectRepository::save(
    const PlanningProject &project, QString &error)
{
    error.clear();
    const auto existing = std::find_if(
        m_projects.begin(), m_projects.end(),
        [&project](const PlanningProject &candidate) {
            return candidate.id() == project.id();
        });
    if (existing == m_projects.end()) {
        m_projects.push_back(project);
    } else {
        *existing = project;
    }
    return true;
}

std::vector<PlanningProject> InMemoryPlanningProjectRepository::all(
    QString &error) const
{
    error.clear();
    auto projects = m_projects;
    std::sort(
        projects.begin(), projects.end(),
        [](const PlanningProject &left, const PlanningProject &right) {
            return left.updatedAtUtc() > right.updatedAtUtc();
        });
    return projects;
}

std::optional<PlanningProject>
InMemoryPlanningProjectRepository::findById(
    const QUuid &id, QString &error) const
{
    error.clear();
    const auto existing = std::find_if(
        m_projects.cbegin(), m_projects.cend(),
        [&id](const PlanningProject &candidate) {
            return candidate.id() == id;
        });
    if (existing == m_projects.cend()) {
        return std::nullopt;
    }
    return *existing;
}

} // namespace stukit::planning
