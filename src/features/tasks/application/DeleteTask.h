#pragma once

#include "ITaskRepository.h"

namespace stukit::tasks {

enum class DeleteTaskError
{
    InvalidInput,
    TaskNotFound,
    StorageFailure,
};

class DeleteTask final
{
public:
    explicit DeleteTask(ITaskRepository &repository);

    [[nodiscard]] std::optional<DeleteTaskError> execute(const QUuid &id) const;

private:
    ITaskRepository &m_repository;
};

} // namespace stukit::tasks
