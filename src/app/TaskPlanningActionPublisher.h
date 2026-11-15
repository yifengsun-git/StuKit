#pragma once

#include "IPlanningActionPublisher.h"

namespace stukit::tasks {
class SaveTask;
namespace presentation { class TasksViewModel; }
}

namespace stukit::app {

class TaskPlanningActionPublisher final
    : public planning::IPlanningActionPublisher
{
public:
    TaskPlanningActionPublisher(
        tasks::SaveTask &saveTask,
        tasks::presentation::TasksViewModel &tasksViewModel);

    planning::PlanningActionPublishResult publish(
        const std::vector<planning::PlanningActionDraft> &drafts) override;

private:
    tasks::SaveTask &m_saveTask;
    tasks::presentation::TasksViewModel &m_tasksViewModel;
};

} // namespace stukit::app
