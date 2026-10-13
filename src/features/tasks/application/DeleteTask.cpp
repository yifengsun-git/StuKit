#include "DeleteTask.h"

namespace stukit::tasks {

DeleteTask::DeleteTask(ITaskRepository &repository) : m_repository(repository) {}

std::optional<DeleteTaskError> DeleteTask::execute(const QUuid &id) const
{
    if (id.isNull()) {
        return DeleteTaskError::InvalidInput;
    }
    if (!m_repository.findById(id)) {
        return DeleteTaskError::TaskNotFound;
    }
    if (!m_repository.remove(id)) {
        return DeleteTaskError::StorageFailure;
    }
    return std::nullopt;
}

} // namespace stukit::tasks
