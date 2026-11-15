#pragma once

#include <QString>

#include <vector>

namespace stukit::planning {

struct PlanningActionDraft
{
    QString title;
    QString description;
};

struct PlanningActionPublishResult
{
    int publishedCount{0};
    QString error;

    [[nodiscard]] bool succeeded() const noexcept
    {
        return error.isEmpty();
    }
};

class IPlanningActionPublisher
{
public:
    virtual ~IPlanningActionPublisher() = default;

    virtual PlanningActionPublishResult publish(
        const std::vector<PlanningActionDraft> &drafts) = 0;
};

} // namespace stukit::planning
