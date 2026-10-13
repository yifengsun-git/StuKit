#include "TaskModule.h"

#include "ChangeTaskStatus.h"
#include "DeleteTask.h"
#include "GetTaskOverview.h"
#include "SaveTask.h"
#include "ScheduleCourseLookup.h"
#include "ScanTaskReminders.h"
#include "SqliteTaskRepository.h"
#include "SqliteTaskCategoryStore.h"
#include "SqliteReminderDeliveryStore.h"
#include "TasksViewModel.h"
#include "repositories/SqliteAcademicTermRepository.h"
#include "repositories/SqliteCourseRepository.h"

#include <utility>

namespace stukit::app {

TaskModule::TaskModule(QSqlDatabase database, QTimeZone displayTimeZone)
    : m_database(std::move(database))
    , m_displayTimeZone(std::move(displayTimeZone))
{
}

TaskModule::~TaskModule() = default;

bool TaskModule::initialize()
{
    reset();
    m_lastError.clear();
    if (!m_database.isOpen() || !m_displayTimeZone.isValid()) {
        m_lastError = QStringLiteral("Task database or time zone is invalid");
        return false;
    }
    m_taskRepository = std::make_unique<tasks::SqliteTaskRepository>(m_database);
    m_categoryStore = std::make_unique<tasks::SqliteTaskCategoryStore>(m_database);
    m_termRepository = std::make_unique<schedule::SqliteAcademicTermRepository>(m_database);
    m_courseRepository = std::make_unique<schedule::SqliteCourseRepository>(m_database);
    m_courseLookup = std::make_unique<tasks::ScheduleCourseLookup>(
        *m_termRepository, *m_courseRepository);
    m_deliveryStore = std::make_unique<tasks::SqliteReminderDeliveryStore>(
        m_database);
    m_saveTask = std::make_unique<tasks::SaveTask>(
        *m_taskRepository, m_courseLookup.get());
    m_changeStatus = std::make_unique<tasks::ChangeTaskStatus>(*m_taskRepository);
    m_deleteTask = std::make_unique<tasks::DeleteTask>(*m_taskRepository);
    m_getOverview = std::make_unique<tasks::GetTaskOverview>(*m_taskRepository);
    m_viewModel = std::make_unique<tasks::presentation::TasksViewModel>(
        *m_saveTask, *m_changeStatus, *m_deleteTask, *m_getOverview,
        *m_courseLookup, *m_categoryStore, m_displayTimeZone);
    m_scanReminders = std::make_unique<tasks::ScanTaskReminders>(
        *m_taskRepository, *m_deliveryStore, *m_viewModel);
    m_viewModel->setReminderScanner(m_scanReminders.get());
    m_viewModel->load();
    if (m_viewModel->hasError()) {
        m_lastError = m_viewModel->errorMessage();
        reset();
        return false;
    }
    return true;
}

tasks::presentation::TasksViewModel *TaskModule::viewModel() const noexcept
{
    return m_viewModel.get();
}

const QString &TaskModule::lastError() const noexcept { return m_lastError; }

void TaskModule::reset()
{
    m_scanReminders.reset();
    m_viewModel.reset();
    m_getOverview.reset();
    m_deleteTask.reset();
    m_changeStatus.reset();
    m_saveTask.reset();
    m_courseLookup.reset();
    m_deliveryStore.reset();
    m_courseRepository.reset();
    m_termRepository.reset();
    m_taskRepository.reset();
    m_categoryStore.reset();
}

} // namespace stukit::app
