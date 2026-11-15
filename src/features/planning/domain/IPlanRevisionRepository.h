#pragma once

#include "PlanRevision.h"

#include <QString>

#include <optional>
#include <vector>

namespace stukit::planning {

class IPlanRevisionRepository
{
public:
    virtual ~IPlanRevisionRepository() = default;

    virtual bool save(const PlanRevision &revision, QString &error) = 0;
    virtual std::vector<PlanRevision> forProject(
        const QUuid &projectId, QString &error) const = 0;
    virtual std::optional<PlanRevision> latest(
        const QUuid &projectId, QString &error) const = 0;
};

} // namespace stukit::planning
