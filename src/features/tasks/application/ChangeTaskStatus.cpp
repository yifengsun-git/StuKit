#include "ChangeTaskStatus.h"

#include <utility>

namespace stukit::tasks {

ChangeTaskStatus::ChangeTaskStatus(ITaskRepository &repository)
    : m_repository(repository)
{
}

ChangeTaskStatusResult ChangeTaskStatus::execute(
    const ChangeTaskStatusRequest &request) const
{
    if (request.id.isNull()
        || !request.nowUtc.isValid()
        || request.nowUtc.timeSpec() != Qt::UTC) {
        return ChangeTaskStatusError::InvalidInput;
    }
    const auto existing = m_repository.findById(request.id);
    if (!existing) {
        return ChangeTaskStatusError::TaskNotFound;
    }

    TaskItemData data = existing->data();
    switch (request.action) {
    case TaskStatusAction::Complete:
        if (data.status != TaskStatus::Pending) {
            return ChangeTaskStatusError::InvalidTransition;
        }
        data.status = TaskStatus::Completed;
        data.completedAtUtc = request.nowUtc;
        break;
    case TaskStatusAction::Reopen:
        if (data.status != TaskStatus::Completed) {
            return ChangeTaskStatusError::InvalidTransition;
        }
        data.status = TaskStatus::Pending;
        data.completedAtUtc.reset();
        break;
    case TaskStatusAction::Archive:
        if (data.status != TaskStatus::Completed) {
            return ChangeTaskStatusError::InvalidTransition;
        }
        data.status = TaskStatus::Archived;
        break;
    case TaskStatusAction::Restore:
        if (data.status != TaskStatus::Archived) {
            return ChangeTaskStatusError::InvalidTransition;
        }
        data.status = TaskStatus::Completed;
        break;
    }

    auto changed = TaskItem::create(std::move(data));
    if (!changed) {
        return ChangeTaskStatusError::InvalidInput;
    }
    if (!m_repository.save(*changed)) {
        return ChangeTaskStatusError::StorageFailure;
    }
    return *changed;
}

} // namespace stukit::tasks
