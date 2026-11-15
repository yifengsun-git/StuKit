#include "PlanningProject.h"

#include <utility>

namespace stukit::planning {
namespace {

bool isUtc(const QDateTime &value)
{
    return value.isValid() && value.timeSpec() == Qt::UTC;
}

bool isValidStatus(PlanningProjectStatus status)
{
    switch (status) {
    case PlanningProjectStatus::Draft:
    case PlanningProjectStatus::Active:
    case PlanningProjectStatus::Archived:
        return true;
    }
    return false;
}

} // namespace

std::optional<PlanningProject> PlanningProject::create(
    PlanningProjectData data)
{
    data.title = data.title.trimmed();
    data.objective = data.objective.trimmed();
    data.constraints = data.constraints.trimmed();

    if (data.id.isNull()
        || data.title.isEmpty()
        || data.title.size() > MaximumTitleLength
        || data.objective.isEmpty()
        || data.objective.size() > MaximumObjectiveLength
        || data.constraints.size() > MaximumConstraintsLength
        || !isValidStatus(data.status)
        || !isUtc(data.createdAtUtc)
        || !isUtc(data.updatedAtUtc)
        || data.updatedAtUtc < data.createdAtUtc) {
        return std::nullopt;
    }

    return PlanningProject(std::move(data));
}

PlanningProject::PlanningProject(PlanningProjectData data)
    : m_data(std::move(data))
{
}

const QUuid &PlanningProject::id() const noexcept { return m_data.id; }
const QString &PlanningProject::title() const noexcept { return m_data.title; }
const QString &PlanningProject::objective() const noexcept
{
    return m_data.objective;
}
const QString &PlanningProject::constraints() const noexcept
{
    return m_data.constraints;
}
PlanningProjectStatus PlanningProject::status() const noexcept
{
    return m_data.status;
}
const QDateTime &PlanningProject::createdAtUtc() const noexcept
{
    return m_data.createdAtUtc;
}
const QDateTime &PlanningProject::updatedAtUtc() const noexcept
{
    return m_data.updatedAtUtc;
}
bool PlanningProject::isArchived() const noexcept
{
    return m_data.status == PlanningProjectStatus::Archived;
}
PlanningProjectData PlanningProject::data() const { return m_data; }

} // namespace stukit::planning
