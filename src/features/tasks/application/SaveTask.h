#pragma once

#include "ITaskRepository.h"
#include "ICourseLookup.h"

#include <variant>

namespace stukit::tasks {

struct SaveTaskRequest
{
    QUuid id;
    QString title;
    QString description;
    std::optional<QDateTime> dueAtUtc;
    std::optional<QDateTime> reminderAtUtc;
    TaskPriority priority{TaskPriority::Normal};
    QString categoryName{QStringLiteral("作业")};
    std::optional<QUuid> courseId;
    QString courseNameSnapshot;
    QDateTime nowUtc;
};

enum class SaveTaskError
{
    InvalidInput,
    TaskNotFound,
    CourseNotFound,
    StorageFailure,
};

using SaveTaskResult = std::variant<TaskItem, SaveTaskError>;

class SaveTask final
{
public:
    explicit SaveTask(
        ITaskRepository &repository,
        ICourseLookup *courseLookup = nullptr);

    [[nodiscard]] SaveTaskResult execute(const SaveTaskRequest &request) const;

private:
    ITaskRepository &m_repository;
    ICourseLookup *m_courseLookup;
};

} // namespace stukit::tasks
