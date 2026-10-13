#include "TaskItem.h"

#include <utility>

namespace stukit::tasks {
namespace {

bool isUtc(const QDateTime &value)
{
    return value.isValid() && value.timeSpec() == Qt::UTC;
}

bool validPriority(TaskPriority priority)
{
    switch (priority) {
    case TaskPriority::Low:
    case TaskPriority::Normal:
    case TaskPriority::High:
    case TaskPriority::Urgent:
        return true;
    }
    return false;
}

bool validStatus(TaskStatus status)
{
    switch (status) {
    case TaskStatus::Pending:
    case TaskStatus::Completed:
    case TaskStatus::Archived:
        return true;
    }
    return false;
}

} // namespace

std::optional<TaskItem> TaskItem::create(TaskItemData data)
{
    data.title = data.title.trimmed();
    data.description = data.description.trimmed();
    data.categoryName = data.categoryName.trimmed();
    data.courseNameSnapshot = data.courseNameSnapshot.trimmed();

    const bool hasCourse = data.courseId.has_value();
    const bool validCourse = hasCourse
        ? !data.courseId->isNull()
            && !data.courseNameSnapshot.isEmpty()
            && data.courseNameSnapshot.size() <= MaximumCourseNameLength
        : data.courseNameSnapshot.isEmpty();
    const bool completionMatchesStatus = data.status == TaskStatus::Pending
        ? !data.completedAtUtc.has_value()
        : data.completedAtUtc.has_value();

    if (data.id.isNull()
        || data.title.isEmpty()
        || data.title.size() > MaximumTitleLength
        || data.description.size() > MaximumDescriptionLength
        || !validPriority(data.priority)
        || data.categoryName.isEmpty()
        || data.categoryName.size() > MaximumCategoryNameLength
        || !validStatus(data.status)
        || !isUtc(data.createdAtUtc)
        || (data.dueAtUtc && !isUtc(*data.dueAtUtc))
        || (data.reminderAtUtc && !isUtc(*data.reminderAtUtc))
        || (data.completedAtUtc && !isUtc(*data.completedAtUtc))
        || (data.reminderAtUtc && !data.dueAtUtc)
        || (data.reminderAtUtc && *data.reminderAtUtc > *data.dueAtUtc)
        || (data.completedAtUtc && *data.completedAtUtc < data.createdAtUtc)
        || !completionMatchesStatus
        || !validCourse) {
        return std::nullopt;
    }

    return TaskItem(std::move(data));
}

TaskItem::TaskItem(TaskItemData data) : m_data(std::move(data)) {}

const QUuid &TaskItem::id() const noexcept { return m_data.id; }
const QString &TaskItem::title() const noexcept { return m_data.title; }
const QString &TaskItem::description() const noexcept { return m_data.description; }
const std::optional<QDateTime> &TaskItem::dueAtUtc() const noexcept
{
    return m_data.dueAtUtc;
}
const std::optional<QDateTime> &TaskItem::reminderAtUtc() const noexcept
{
    return m_data.reminderAtUtc;
}
TaskPriority TaskItem::priority() const noexcept { return m_data.priority; }
const QString &TaskItem::categoryName() const noexcept
{
    return m_data.categoryName;
}
TaskStatus TaskItem::status() const noexcept { return m_data.status; }
const std::optional<QUuid> &TaskItem::courseId() const noexcept
{
    return m_data.courseId;
}
const QString &TaskItem::courseNameSnapshot() const noexcept
{
    return m_data.courseNameSnapshot;
}
const std::optional<QDateTime> &TaskItem::completedAtUtc() const noexcept
{
    return m_data.completedAtUtc;
}
const QDateTime &TaskItem::createdAtUtc() const noexcept
{
    return m_data.createdAtUtc;
}

TaskItemData TaskItem::data() const { return m_data; }

bool TaskItem::isCompleted() const noexcept
{
    return m_data.status == TaskStatus::Completed
        || m_data.status == TaskStatus::Archived;
}

bool TaskItem::isArchived() const noexcept
{
    return m_data.status == TaskStatus::Archived;
}

bool TaskItem::isOverdue(const QDateTime &nowUtc) const noexcept
{
    return m_data.status == TaskStatus::Pending
        && m_data.dueAtUtc.has_value()
        && isUtc(nowUtc)
        && *m_data.dueAtUtc < nowUtc;
}

bool TaskItem::reminderIsDue(const QDateTime &nowUtc) const noexcept
{
    return m_data.status == TaskStatus::Pending
        && m_data.reminderAtUtc.has_value()
        && isUtc(nowUtc)
        && *m_data.reminderAtUtc <= nowUtc;
}

} // namespace stukit::tasks
