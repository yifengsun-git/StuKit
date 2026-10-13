#include "GetTaskOverview.h"

#include <algorithm>

namespace stukit::tasks {
namespace {

bool matchesKeyword(const TaskItem &task, const QString &keyword)
{
    return keyword.isEmpty()
        || task.title().contains(keyword, Qt::CaseInsensitive)
        || task.description().contains(keyword, Qt::CaseInsensitive)
        || task.courseNameSnapshot().contains(keyword, Qt::CaseInsensitive);
}

bool before(const TaskItem &left, const TaskItem &right)
{
    if (left.status() != right.status()) {
        return static_cast<int>(left.status()) < static_cast<int>(right.status());
    }
    if (left.dueAtUtc().has_value() != right.dueAtUtc().has_value()) {
        return left.dueAtUtc().has_value();
    }
    if (left.dueAtUtc() && right.dueAtUtc()
        && *left.dueAtUtc() != *right.dueAtUtc()) {
        return *left.dueAtUtc() < *right.dueAtUtc();
    }
    if (left.priority() != right.priority()) {
        return static_cast<int>(left.priority())
            > static_cast<int>(right.priority());
    }
    return left.createdAtUtc() < right.createdAtUtc();
}

} // namespace

GetTaskOverview::GetTaskOverview(ITaskRepository &repository)
    : m_repository(repository)
{
}

GetTaskOverviewResult GetTaskOverview::execute(
    TaskFilter filter,
    const QDateTime &nowUtc) const
{
    if (!nowUtc.isValid() || nowUtc.timeSpec() != Qt::UTC
        || (filter.courseId && filter.courseId->isNull())) {
        return GetTaskOverviewError::InvalidInput;
    }
    filter.keyword = filter.keyword.trimmed();
    const auto stored = m_repository.findAll();
    if (!stored) {
        return GetTaskOverviewError::StorageFailure;
    }

    TaskOverview overview;
    for (const TaskItem &task : *stored) {
        if ((!filter.categoryName.isEmpty()
             && task.categoryName().compare(
                    filter.categoryName, Qt::CaseInsensitive) != 0)
            || (filter.courseId && task.courseId() != filter.courseId)
            || !matchesKeyword(task, filter.keyword)) {
            continue;
        }
        if (task.status() == TaskStatus::Pending) {
            ++overview.pendingCount;
            if (task.isOverdue(nowUtc)) {
                ++overview.overdueCount;
            }
        } else {
            ++overview.completedCount;
            if (task.status() == TaskStatus::Archived) {
                ++overview.archivedCount;
            }
        }

        if (!filter.includeArchived && task.isArchived()) {
            continue;
        }
        overview.tasks.push_back(task);
    }

    const int total = overview.pendingCount + overview.completedCount;
    if (total > 0) {
        overview.completionRate = static_cast<double>(overview.completedCount)
            / static_cast<double>(total);
    }
    std::ranges::stable_sort(overview.tasks, before);
    return overview;
}

} // namespace stukit::tasks
