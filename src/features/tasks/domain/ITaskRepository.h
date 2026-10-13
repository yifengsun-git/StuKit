#pragma once

#include "TaskItem.h"

#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::tasks {

class ITaskRepository
{
public:
    virtual ~ITaskRepository() = default;

    [[nodiscard]] virtual bool save(const TaskItem &task) = 0;
    [[nodiscard]] virtual bool remove(const QUuid &id) = 0;
    [[nodiscard]] virtual std::optional<TaskItem> findById(
        const QUuid &id) const = 0;
    [[nodiscard]] virtual std::optional<std::vector<TaskItem>> findAll() const = 0;
};

} // namespace stukit::tasks
