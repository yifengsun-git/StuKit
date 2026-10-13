#pragma once

#include "ITaskRepository.h"

#include <QString>

#include <variant>

namespace stukit::tasks {

struct TaskFilter
{
    bool includeArchived{false};
    QString categoryName;
    std::optional<QUuid> courseId;
    QString keyword;
};

struct TaskOverview
{
    std::vector<TaskItem> tasks;
    int pendingCount{0};
    int completedCount{0};
    int archivedCount{0};
    int overdueCount{0};
    double completionRate{0.0};
};

enum class GetTaskOverviewError
{
    InvalidInput,
    StorageFailure,
};

using GetTaskOverviewResult = std::variant<TaskOverview, GetTaskOverviewError>;

class GetTaskOverview final
{
public:
    explicit GetTaskOverview(ITaskRepository &repository);

    [[nodiscard]] GetTaskOverviewResult execute(
        TaskFilter filter,
        const QDateTime &nowUtc) const;

private:
    ITaskRepository &m_repository;
};

} // namespace stukit::tasks
