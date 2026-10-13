#include "SaveTask.h"

namespace stukit::tasks {

SaveTask::SaveTask(ITaskRepository &repository, ICourseLookup *courseLookup)
    : m_repository(repository)
    , m_courseLookup(courseLookup)
{
}

SaveTaskResult SaveTask::execute(const SaveTaskRequest &request) const
{
    const bool creating = request.id.isNull();
    TaskStatus status = TaskStatus::Pending;
    std::optional<QDateTime> completedAtUtc;
    QDateTime createdAtUtc = request.nowUtc;
    QUuid id = request.id;

    if (creating) {
        id = QUuid::createUuid();
    } else {
        const auto existing = m_repository.findById(id);
        if (!existing) {
            return SaveTaskError::TaskNotFound;
        }
        status = existing->status();
        completedAtUtc = existing->completedAtUtc();
        createdAtUtc = existing->createdAtUtc();
    }

    QString courseNameSnapshot = request.courseNameSnapshot;
    if (request.courseId && m_courseLookup) {
        const auto reference = m_courseLookup->findCourse(*request.courseId);
        if (std::holds_alternative<CourseLookupError>(reference)) {
            return std::get<CourseLookupError>(reference)
                    == CourseLookupError::NotFound
                ? SaveTaskError::CourseNotFound
                : SaveTaskError::StorageFailure;
        }
        courseNameSnapshot = std::get<CourseReference>(reference).name;
    }

    auto task = TaskItem::create(TaskItemData{
        .id = id,
        .title = request.title,
        .description = request.description,
        .dueAtUtc = request.dueAtUtc,
        .reminderAtUtc = request.reminderAtUtc,
        .priority = request.priority,
        .categoryName = request.categoryName,
        .status = status,
        .courseId = request.courseId,
        .courseNameSnapshot = courseNameSnapshot,
        .completedAtUtc = completedAtUtc,
        .createdAtUtc = createdAtUtc,
    });
    if (!task) {
        return SaveTaskError::InvalidInput;
    }
    if (!m_repository.save(*task)) {
        return SaveTaskError::StorageFailure;
    }
    return *task;
}

} // namespace stukit::tasks
