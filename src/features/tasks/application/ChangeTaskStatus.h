#pragma once

#include "ITaskRepository.h"

#include <variant>

namespace stukit::tasks {

enum class TaskStatusAction
{
    Complete,
    Reopen,
    Archive,
    Restore,
};

struct ChangeTaskStatusRequest
{
    QUuid id;
    TaskStatusAction action{TaskStatusAction::Complete};
    QDateTime nowUtc;
};

enum class ChangeTaskStatusError
{
    InvalidInput,
    TaskNotFound,
    InvalidTransition,
    StorageFailure,
};

using ChangeTaskStatusResult = std::variant<TaskItem, ChangeTaskStatusError>;

class ChangeTaskStatus final
{
public:
    explicit ChangeTaskStatus(ITaskRepository &repository);

    [[nodiscard]] ChangeTaskStatusResult execute(
        const ChangeTaskStatusRequest &request) const;

private:
    ITaskRepository &m_repository;
};

} // namespace stukit::tasks
