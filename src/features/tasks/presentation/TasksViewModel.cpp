#include "TasksViewModel.h"

#include <QDateTime>
#include <QVariantMap>
#include <QTimer>

#include <cmath>
#include <utility>

namespace stukit::tasks::presentation {
namespace {

QString priorityText(TaskPriority priority)
{
    switch (priority) {
    case TaskPriority::Low: return QStringLiteral("低");
    case TaskPriority::Normal: return QStringLiteral("普通");
    case TaskPriority::High: return QStringLiteral("高");
    case TaskPriority::Urgent: return QStringLiteral("紧急");
    }
    return {};
}

QString statusText(TaskStatus status)
{
    switch (status) {
    case TaskStatus::Pending: return QStringLiteral("待完成");
    case TaskStatus::Completed: return QStringLiteral("已完成");
    case TaskStatus::Archived: return QStringLiteral("已归档");
    }
    return {};
}

QString groupLabel(
    const TaskItem &task,
    const QDateTime &nowUtc,
    const QTimeZone &displayTimeZone)
{
    if (task.status() == TaskStatus::Completed) {
        return QStringLiteral("已完成");
    }
    if (task.status() == TaskStatus::Archived) {
        return QStringLiteral("已归档");
    }
    if (task.isOverdue(nowUtc)) {
        return QStringLiteral("已逾期");
    }
    if (!task.dueAtUtc()) {
        return QStringLiteral("无截止时间");
    }
    const QDate today = nowUtc.toTimeZone(displayTimeZone).date();
    const QDate dueDate = task.dueAtUtc()->toTimeZone(displayTimeZone).date();
    const qint64 daysUntilDue = today.daysTo(dueDate);
    if (daysUntilDue == 0) {
        return QStringLiteral("今天");
    }
    if (daysUntilDue <= 7) {
        return QStringLiteral("未来 7 天");
    }
    return QStringLiteral("更晚");
}

QString useCaseError(SaveTaskError error)
{
    switch (error) {
    case SaveTaskError::InvalidInput: return QStringLiteral("任务内容或时间格式不正确");
    case SaveTaskError::TaskNotFound: return QStringLiteral("要编辑的任务不存在");
    case SaveTaskError::CourseNotFound: return QStringLiteral("所选课程不存在或学期已归档");
    case SaveTaskError::StorageFailure: return QStringLiteral("任务保存失败");
    }
    return QStringLiteral("任务保存失败");
}

} // namespace

TasksViewModel::TasksViewModel(
    SaveTask &saveTask,
    ChangeTaskStatus &changeStatus,
    DeleteTask &deleteTask,
    GetTaskOverview &getOverview,
    ICourseLookup &courseLookup,
    ITaskCategoryStore &categoryStore,
    QTimeZone displayTimeZone,
    QObject *parent)
    : QObject(parent)
    , m_saveTask(saveTask)
    , m_changeStatus(changeStatus)
    , m_deleteTask(deleteTask)
    , m_getOverview(getOverview)
    , m_courseLookup(courseLookup)
    , m_categoryStore(categoryStore)
    , m_displayTimeZone(std::move(displayTimeZone))
{
}

QVariantList TasksViewModel::tasks() const
{
    QVariantList result;
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    for (const TaskItem &task : m_overview.tasks) {
        const QString dueText = task.dueAtUtc()
            ? task.dueAtUtc()->toTimeZone(m_displayTimeZone).toString(
                QStringLiteral("yyyy-MM-dd HH:mm")) : QString{};
        result.push_back(QVariantMap{
            {QStringLiteral("id"), task.id().toString(QUuid::WithoutBraces)},
            {QStringLiteral("title"), task.title()},
            {QStringLiteral("description"), task.description()},
            {QStringLiteral("dueText"), dueText},
            {QStringLiteral("reminderText"), task.reminderAtUtc()
                ? task.reminderAtUtc()->toTimeZone(m_displayTimeZone).toString(
                    QStringLiteral("yyyy-MM-dd HH:mm")) : QString{}},
            {QStringLiteral("priority"), static_cast<int>(task.priority())},
            {QStringLiteral("priorityText"), priorityText(task.priority())},
            {QStringLiteral("categoryText"), task.categoryName()},
            {QStringLiteral("status"), static_cast<int>(task.status())},
            {QStringLiteral("statusText"), statusText(task.status())},
            {QStringLiteral("groupLabel"), groupLabel(
                 task, nowUtc, m_displayTimeZone)},
            {QStringLiteral("courseId"), task.courseId()
                ? task.courseId()->toString(QUuid::WithoutBraces) : QString{}},
            {QStringLiteral("courseName"), task.courseNameSnapshot()},
            {QStringLiteral("overdue"), task.isOverdue(nowUtc)},
            {QStringLiteral("completed"), task.isCompleted()},
            {QStringLiteral("archived"), task.isArchived()},
        });
    }
    return result;
}

QVariantList TasksViewModel::courseOptions() const
{
    QVariantList result{{QVariantMap{{QStringLiteral("id"), QString{}},
                                    {QStringLiteral("name"), QStringLiteral("不绑定课程")},
                                    {QStringLiteral("label"), QStringLiteral("不绑定课程")}}}};
    for (const CourseReference &course : m_courses) {
        result.push_back(QVariantMap{
            {QStringLiteral("id"), course.id.toString(QUuid::WithoutBraces)},
            {QStringLiteral("name"), course.name},
            {QStringLiteral("label"), QStringLiteral("%1 · %2")
                .arg(course.termName, course.name)},
        });
    }
    return result;
}

QStringList TasksViewModel::categoryOptions() const { return m_categories; }

int TasksViewModel::pendingCount() const noexcept { return m_overview.pendingCount; }
int TasksViewModel::completedCount() const noexcept { return m_overview.completedCount; }
int TasksViewModel::overdueCount() const noexcept { return m_overview.overdueCount; }
QString TasksViewModel::completionRateText() const
{
    return QStringLiteral("%1%").arg(
        qRound(m_overview.completionRate * 100.0));
}
bool TasksViewModel::includeArchived() const noexcept { return m_includeArchived; }
int TasksViewModel::categoryFilter() const noexcept { return m_categoryFilter; }
const QString &TasksViewModel::keyword() const noexcept { return m_keyword; }
bool TasksViewModel::hasError() const noexcept { return !m_errorMessage.isEmpty(); }
const QString &TasksViewModel::errorMessage() const noexcept { return m_errorMessage; }
QVariantList TasksViewModel::reminders() const { return m_reminders; }

void TasksViewModel::setReminderScanner(ScanTaskReminders *scanner)
{
    m_reminderScanner = scanner;
    if (!scanner) return;
    auto *timer = new QTimer(this);
    timer->setInterval(60'000);
    connect(timer, &QTimer::timeout, this, &TasksViewModel::scanReminders);
    timer->start();
    scanReminders();
}

bool TasksViewModel::deliver(const TaskReminder &reminder)
{
    m_reminders.push_back(QVariantMap{
        {QStringLiteral("taskId"), reminder.taskId.toString(QUuid::WithoutBraces)},
        {QStringLiteral("title"), reminder.title},
        {QStringLiteral("courseName"), reminder.courseName},
        {QStringLiteral("dueText"), reminder.dueAtUtc.toTimeZone(
             m_displayTimeZone).toString(QStringLiteral("yyyy-MM-dd HH:mm"))},
    });
    emit remindersChanged();
    return true;
}

void TasksViewModel::load()
{
    TaskFilter filter;
    filter.includeArchived = m_includeArchived;
    if (m_categoryFilter >= 0 && m_categoryFilter < m_categories.size()) {
        filter.categoryName = m_categories.at(m_categoryFilter);
    }
    filter.keyword = m_keyword;
    const auto overview = m_getOverview.execute(
        std::move(filter), QDateTime::currentDateTimeUtc());
    const auto courses = m_courseLookup.availableCourses();
    const auto categories = m_categoryStore.categories();
    if (std::holds_alternative<GetTaskOverviewError>(overview)
        || std::holds_alternative<CourseLookupError>(courses)
        || !categories) {
        setError(QStringLiteral("无法读取任务或课程数据"));
        return;
    }
    m_overview = std::get<TaskOverview>(overview);
    m_courses = std::get<std::vector<CourseReference>>(courses);
    m_categories = *categories;
    clearError();
    emit dataChanged();
}

void TasksViewModel::setIncludeArchived(bool include)
{
    if (m_includeArchived == include) return;
    m_includeArchived = include;
    emit filterChanged();
    load();
}

void TasksViewModel::setCategoryFilter(int category)
{
    const int normalized = category >= 0 && category < m_categories.size()
        ? category : -1;
    if (m_categoryFilter == normalized) return;
    m_categoryFilter = normalized;
    emit filterChanged();
    load();
}

bool TasksViewModel::addCategory(const QString &name)
{
    const QString normalized = name.trimmed();
    if (normalized.isEmpty() || normalized.size() > TaskItem::MaximumCategoryNameLength
        || m_categories.contains(normalized, Qt::CaseInsensitive)
        || !m_categoryStore.addCategory(normalized)) {
        setError(QStringLiteral("分类名称为空、过长或已经存在"));
        return false;
    }
    load();
    return true;
}

void TasksViewModel::setKeyword(const QString &keyword)
{
    const QString normalized = keyword.trimmed();
    if (m_keyword == normalized) return;
    m_keyword = normalized;
    emit filterChanged();
    load();
}

std::optional<QDateTime> TasksViewModel::parseLocalTime(
    const QString &text, bool &valid) const
{
    if (text.trimmed().isEmpty()) {
        valid = true;
        return std::nullopt;
    }
    QDateTime local = QDateTime::fromString(
        text.trimmed(), QStringLiteral("yyyy-MM-dd HH:mm"));
    local.setTimeZone(m_displayTimeZone);
    valid = local.isValid();
    return valid ? std::optional<QDateTime>(local.toUTC()) : std::nullopt;
}

bool TasksViewModel::save(
    const QString &id, const QString &title, const QString &description,
    const QString &dueLocalText, const QString &reminderLocalText,
    int priority, const QString &categoryName, const QString &courseId)
{
    bool dueValid = false;
    bool reminderValid = false;
    const auto due = parseLocalTime(dueLocalText, dueValid);
    const auto reminder = parseLocalTime(reminderLocalText, reminderValid);
    if (!dueValid || !reminderValid) {
        setError(QStringLiteral("时间格式应为 yyyy-MM-dd HH:mm"));
        return false;
    }
    const QString course = courseId.trimmed();
    const auto result = m_saveTask.execute(SaveTaskRequest{
        .id = QUuid(id.trimmed()),
        .title = title,
        .description = description,
        .dueAtUtc = due,
        .reminderAtUtc = reminder,
        .priority = static_cast<TaskPriority>(priority),
        .categoryName = categoryName,
        .courseId = course.isEmpty()
            ? std::nullopt : std::optional<QUuid>(QUuid(course)),
        .courseNameSnapshot = {},
        .nowUtc = QDateTime::currentDateTimeUtc(),
    });
    if (std::holds_alternative<SaveTaskError>(result)) {
        setError(useCaseError(std::get<SaveTaskError>(result)));
        return false;
    }
    load();
    return true;
}

bool TasksViewModel::change(const QString &id, TaskStatusAction action)
{
    const auto result = m_changeStatus.execute(
        {QUuid(id.trimmed()), action, QDateTime::currentDateTimeUtc()});
    if (std::holds_alternative<ChangeTaskStatusError>(result)) {
        setError(QStringLiteral("无法更新任务状态"));
        return false;
    }
    load();
    return true;
}

bool TasksViewModel::complete(const QString &id) { return change(id, TaskStatusAction::Complete); }
bool TasksViewModel::reopen(const QString &id) { return change(id, TaskStatusAction::Reopen); }
bool TasksViewModel::archive(const QString &id) { return change(id, TaskStatusAction::Archive); }
bool TasksViewModel::restore(const QString &id) { return change(id, TaskStatusAction::Restore); }

bool TasksViewModel::remove(const QString &id)
{
    const auto error = m_deleteTask.execute(QUuid(id.trimmed()));
    if (error) {
        setError(QStringLiteral("无法删除任务"));
        return false;
    }
    load();
    return true;
}

void TasksViewModel::dismissReminder(int index)
{
    if (index < 0 || index >= m_reminders.size()) return;
    m_reminders.removeAt(index);
    emit remindersChanged();
}

void TasksViewModel::scanReminders()
{
    if (!m_reminderScanner) return;
    const auto result = m_reminderScanner->execute(
        QDateTime::currentDateTimeUtc());
    if (std::holds_alternative<ScanTaskRemindersError>(result)) {
        setError(QStringLiteral("任务提醒扫描失败"));
    }
}

void TasksViewModel::setError(QString message)
{
    m_errorMessage = std::move(message);
    emit errorChanged();
}

void TasksViewModel::clearError()
{
    if (m_errorMessage.isEmpty()) return;
    m_errorMessage.clear();
    emit errorChanged();
}

} // namespace stukit::tasks::presentation
