#pragma once

#include "TaskTypes.h"

#include <QDateTime>
#include <QString>
#include <QUuid>

#include <optional>

namespace stukit::tasks {

struct TaskItemData
{
    QUuid id;
    QString title;
    QString description;
    std::optional<QDateTime> dueAtUtc;
    std::optional<QDateTime> reminderAtUtc;
    TaskPriority priority{TaskPriority::Normal};
    QString categoryName;
    TaskStatus status{TaskStatus::Pending};
    std::optional<QUuid> courseId;
    QString courseNameSnapshot;
    std::optional<QDateTime> completedAtUtc;
    QDateTime createdAtUtc;
};

class TaskItem final
{
public:
    static constexpr qsizetype MaximumTitleLength = 120;
    static constexpr qsizetype MaximumDescriptionLength = 5000;
    static constexpr qsizetype MaximumCategoryNameLength = 40;
    static constexpr qsizetype MaximumCourseNameLength = 80;

    static std::optional<TaskItem> create(TaskItemData data);

    [[nodiscard]] const QUuid &id() const noexcept;
    [[nodiscard]] const QString &title() const noexcept;
    [[nodiscard]] const QString &description() const noexcept;
    [[nodiscard]] const std::optional<QDateTime> &dueAtUtc() const noexcept;
    [[nodiscard]] const std::optional<QDateTime> &reminderAtUtc() const noexcept;
    [[nodiscard]] TaskPriority priority() const noexcept;
    [[nodiscard]] const QString &categoryName() const noexcept;
    [[nodiscard]] TaskStatus status() const noexcept;
    [[nodiscard]] const std::optional<QUuid> &courseId() const noexcept;
    [[nodiscard]] const QString &courseNameSnapshot() const noexcept;
    [[nodiscard]] const std::optional<QDateTime> &completedAtUtc() const noexcept;
    [[nodiscard]] const QDateTime &createdAtUtc() const noexcept;
    [[nodiscard]] TaskItemData data() const;

    [[nodiscard]] bool isCompleted() const noexcept;
    [[nodiscard]] bool isArchived() const noexcept;
    [[nodiscard]] bool isOverdue(const QDateTime &nowUtc) const noexcept;
    [[nodiscard]] bool reminderIsDue(const QDateTime &nowUtc) const noexcept;

private:
    explicit TaskItem(TaskItemData data);

    TaskItemData m_data;
};

} // namespace stukit::tasks
