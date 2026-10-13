#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QTimeZone>

#include <memory>

namespace stukit::schedule {
class SqliteAcademicTermRepository;
class SqliteCourseRepository;
}

namespace stukit::tasks {
class ChangeTaskStatus;
class DeleteTask;
class GetTaskOverview;
class SaveTask;
class ScheduleCourseLookup;
class ScanTaskReminders;
class SqliteReminderDeliveryStore;
class SqliteTaskCategoryStore;
class SqliteTaskRepository;
namespace presentation { class TasksViewModel; }
}

namespace stukit::app {

class TaskModule final
{
public:
    TaskModule(QSqlDatabase database, QTimeZone displayTimeZone);
    ~TaskModule();

    [[nodiscard]] bool initialize();
    [[nodiscard]] tasks::presentation::TasksViewModel *viewModel() const noexcept;
    [[nodiscard]] const QString &lastError() const noexcept;

private:
    void reset();

    QSqlDatabase m_database;
    QTimeZone m_displayTimeZone;
    QString m_lastError;
    std::unique_ptr<tasks::SqliteTaskRepository> m_taskRepository;
    std::unique_ptr<tasks::SqliteTaskCategoryStore> m_categoryStore;
    std::unique_ptr<schedule::SqliteAcademicTermRepository> m_termRepository;
    std::unique_ptr<schedule::SqliteCourseRepository> m_courseRepository;
    std::unique_ptr<tasks::ScheduleCourseLookup> m_courseLookup;
    std::unique_ptr<tasks::SqliteReminderDeliveryStore> m_deliveryStore;
    std::unique_ptr<tasks::SaveTask> m_saveTask;
    std::unique_ptr<tasks::ChangeTaskStatus> m_changeStatus;
    std::unique_ptr<tasks::DeleteTask> m_deleteTask;
    std::unique_ptr<tasks::GetTaskOverview> m_getOverview;
    std::unique_ptr<tasks::ScanTaskReminders> m_scanReminders;
    std::unique_ptr<tasks::presentation::TasksViewModel> m_viewModel;
};

} // namespace stukit::app
