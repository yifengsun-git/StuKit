#pragma once

#include "PlanningProject.h"

#include <QString>
#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::planning {

class IPlanningProjectRepository
{
public:
    virtual ~IPlanningProjectRepository() = default;

    virtual bool save(const PlanningProject &project, QString &error) = 0;
    virtual std::vector<PlanningProject> all(QString &error) const = 0;
    virtual std::optional<PlanningProject> findById(
        const QUuid &id, QString &error) const = 0;
};

} // namespace stukit::planning
