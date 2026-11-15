#include "TaskPlanningActionPublisher.h"

#include "SaveTask.h"
#include "TasksViewModel.h"

#include <QDateTime>

#include <variant>

namespace stukit::app {
namespace {

QString publishError(stukit::tasks::SaveTaskError error)
{
    using stukit::tasks::SaveTaskError;
    switch (error) {
    case SaveTaskError::InvalidInput:
        return QStringLiteral("规划行动的标题或说明无效");
    case SaveTaskError::TaskNotFound:
        return QStringLiteral("待办不存在");
    case SaveTaskError::CourseNotFound:
        return QStringLiteral("关联课程不存在");
    case SaveTaskError::StorageFailure:
        return QStringLiteral("待办保存失败");
    }
    return QStringLiteral("待办保存失败");
}

} // namespace

TaskPlanningActionPublisher::TaskPlanningActionPublisher(
    tasks::SaveTask &saveTask,
    tasks::presentation::TasksViewModel &tasksViewModel)
    : m_saveTask(saveTask)
    , m_tasksViewModel(tasksViewModel)
{
}

planning::PlanningActionPublishResult
TaskPlanningActionPublisher::publish(
    const std::vector<planning::PlanningActionDraft> &drafts)
{
    planning::PlanningActionPublishResult result;
    const QString categoryName = QStringLiteral("规划");
    if (!m_tasksViewModel.categoryOptions().contains(
            categoryName, Qt::CaseInsensitive)
        && !m_tasksViewModel.addCategory(categoryName)) {
        result.error = m_tasksViewModel.errorMessage().isEmpty()
            ? QStringLiteral("无法创建“规划”待办分类")
            : m_tasksViewModel.errorMessage();
        return result;
    }
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    for (const planning::PlanningActionDraft &draft : drafts) {
        const tasks::SaveTaskResult saved = m_saveTask.execute(
            tasks::SaveTaskRequest{
                .id = {},
                .title = draft.title,
                .description = draft.description,
                .dueAtUtc = std::nullopt,
                .reminderAtUtc = std::nullopt,
                .priority = tasks::TaskPriority::Normal,
                .categoryName = categoryName,
                .courseId = std::nullopt,
                .courseNameSnapshot = {},
                .nowUtc = nowUtc,
            });
        if (std::holds_alternative<tasks::SaveTaskError>(saved)) {
            result.error = publishError(
                std::get<tasks::SaveTaskError>(saved));
            break;
        }
        ++result.publishedCount;
    }
    if (result.publishedCount > 0) {
        m_tasksViewModel.load();
    }
    return result;
}

} // namespace stukit::app
