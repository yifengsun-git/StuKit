#pragma once

#include "ITaskRepository.h"

#include <algorithm>

namespace stukit::tasks::test {

class InMemoryTaskRepository final : public ITaskRepository
{
public:
    bool save(const TaskItem &task) override
    {
        if (failWrites) {
            return false;
        }
        const auto existing = std::ranges::find_if(
            tasks,
            [&task](const TaskItem &candidate) {
                return candidate.id() == task.id();
            });
        if (existing == tasks.end()) {
            tasks.push_back(task);
        } else {
            *existing = task;
        }
        return true;
    }

    bool remove(const QUuid &id) override
    {
        if (failWrites) {
            return false;
        }
        const auto oldSize = tasks.size();
        std::erase_if(tasks, [&id](const TaskItem &task) {
            return task.id() == id;
        });
        return tasks.size() != oldSize;
    }

    std::optional<TaskItem> findById(const QUuid &id) const override
    {
        const auto found = std::ranges::find_if(
            tasks,
            [&id](const TaskItem &task) { return task.id() == id; });
        return found == tasks.end()
            ? std::nullopt : std::optional<TaskItem>(*found);
    }

    std::optional<std::vector<TaskItem>> findAll() const override
    {
        if (failReads) {
            return std::nullopt;
        }
        return tasks;
    }

    std::vector<TaskItem> tasks;
    bool failReads{false};
    bool failWrites{false};
};

} // namespace stukit::tasks::test
