#include "SqliteTaskRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <utility>

namespace stukit::tasks {
namespace {

QVariant optionalDateTime(const std::optional<QDateTime> &value)
{
    return value
        ? QVariant(value->toString(Qt::ISODateWithMs))
        : QVariant(QMetaType::fromType<QString>());
}

QVariant optionalUuid(const std::optional<QUuid> &value)
{
    return value
        ? QVariant(value->toString(QUuid::WithoutBraces))
        : QVariant(QMetaType::fromType<QString>());
}

std::optional<QDateTime> dateTimeFromNullable(const QVariant &value)
{
    if (value.isNull()) {
        return std::nullopt;
    }
    return QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
}

std::optional<QUuid> uuidFromNullable(const QVariant &value)
{
    if (value.isNull()) {
        return std::nullopt;
    }
    return QUuid(value.toString());
}

int legacyCategoryValue(const QString &name)
{
    if (name == QStringLiteral("考试")) return 1;
    if (name == QStringLiteral("竞赛")) return 2;
    return 0;
}

} // namespace

SqliteTaskRepository::SqliteTaskRepository(QSqlDatabase database)
    : m_database(std::move(database))
{
}

bool SqliteTaskRepository::save(const TaskItem &task)
{
    m_lastError.clear();
    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO tasks(id, title, description, due_at, reminder_at, "
        "priority, category, category_name, status, course_id, course_name_snapshot, "
        "completed_at, created_at) VALUES("
        ":id, :title, :description, :dueAt, :reminderAt, :priority, "
        ":category, :categoryName, :status, :courseId, :courseName, :completedAt, :createdAt) "
        "ON CONFLICT(id) DO UPDATE SET "
        "title = excluded.title, description = excluded.description, "
        "due_at = excluded.due_at, reminder_at = excluded.reminder_at, "
        "priority = excluded.priority, category = excluded.category, "
        "category_name = excluded.category_name, "
        "status = excluded.status, course_id = excluded.course_id, "
        "course_name_snapshot = excluded.course_name_snapshot, "
        "completed_at = excluded.completed_at, created_at = excluded.created_at"));
    query.bindValue(QStringLiteral(":id"),
                    task.id().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":title"), task.title());
    query.bindValue(QStringLiteral(":description"),
                    task.description().isNull()
                        ? QStringLiteral("") : task.description());
    query.bindValue(QStringLiteral(":dueAt"), optionalDateTime(task.dueAtUtc()));
    query.bindValue(QStringLiteral(":reminderAt"),
                    optionalDateTime(task.reminderAtUtc()));
    query.bindValue(QStringLiteral(":priority"), static_cast<int>(task.priority()));
    query.bindValue(QStringLiteral(":category"),
                    legacyCategoryValue(task.categoryName()));
    query.bindValue(QStringLiteral(":categoryName"), task.categoryName());
    query.bindValue(QStringLiteral(":status"), static_cast<int>(task.status()));
    query.bindValue(QStringLiteral(":courseId"), optionalUuid(task.courseId()));
    query.bindValue(QStringLiteral(":courseName"),
                    task.courseNameSnapshot().isNull()
                        ? QStringLiteral("") : task.courseNameSnapshot());
    query.bindValue(QStringLiteral(":completedAt"),
                    optionalDateTime(task.completedAtUtc()));
    query.bindValue(QStringLiteral(":createdAt"),
                    task.createdAtUtc().toString(Qt::ISODateWithMs));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool SqliteTaskRepository::remove(const QUuid &id)
{
    m_lastError.clear();
    if (!m_database.isOpen() || id.isNull()) {
        m_lastError = QStringLiteral("Database is not open or id is invalid");
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM tasks WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id.toString(QUuid::WithoutBraces));
    if (!query.exec() || query.numRowsAffected() != 1) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

std::optional<TaskItem> SqliteTaskRepository::findById(const QUuid &id) const
{
    m_lastError.clear();
    if (!m_database.isOpen() || id.isNull()) {
        m_lastError = QStringLiteral("Database is not open or id is invalid");
        return std::nullopt;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, title, description, due_at, reminder_at, priority, "
        "category, category_name, status, course_id, course_name_snapshot, completed_at, "
        "created_at FROM tasks WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    if (!query.next()) {
        return std::nullopt;
    }
    return taskFromCurrentRow(query);
}

std::optional<std::vector<TaskItem>> SqliteTaskRepository::findAll() const
{
    m_lastError.clear();
    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT id, title, description, due_at, reminder_at, priority, "
            "category, category_name, status, course_id, course_name_snapshot, completed_at, "
            "created_at FROM tasks ORDER BY status, due_at IS NULL, due_at, "
            "priority DESC, created_at"))) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    return tasksFromQuery(query);
}

const QString &SqliteTaskRepository::lastError() const noexcept
{
    return m_lastError;
}

std::optional<TaskItem> SqliteTaskRepository::taskFromCurrentRow(
    const QSqlQuery &query) const
{
    auto task = TaskItem::create(TaskItemData{
        .id = QUuid(query.value(QStringLiteral("id")).toString()),
        .title = query.value(QStringLiteral("title")).toString(),
        .description = query.value(QStringLiteral("description")).toString(),
        .dueAtUtc = dateTimeFromNullable(query.value(QStringLiteral("due_at"))),
        .reminderAtUtc = dateTimeFromNullable(
            query.value(QStringLiteral("reminder_at"))),
        .priority = static_cast<TaskPriority>(
            query.value(QStringLiteral("priority")).toInt()),
        .categoryName = query.value(QStringLiteral("category_name")).toString(),
        .status = static_cast<TaskStatus>(
            query.value(QStringLiteral("status")).toInt()),
        .courseId = uuidFromNullable(query.value(QStringLiteral("course_id"))),
        .courseNameSnapshot = query.value(
            QStringLiteral("course_name_snapshot")).toString(),
        .completedAtUtc = dateTimeFromNullable(
            query.value(QStringLiteral("completed_at"))),
        .createdAtUtc = QDateTime::fromString(
            query.value(QStringLiteral("created_at")).toString(),
            Qt::ISODateWithMs),
    });
    if (!task) {
        m_lastError = QStringLiteral("Database contains an invalid task");
    }
    return task;
}

std::optional<std::vector<TaskItem>> SqliteTaskRepository::tasksFromQuery(
    QSqlQuery &query) const
{
    std::vector<TaskItem> tasks;
    while (query.next()) {
        auto task = taskFromCurrentRow(query);
        if (!task) {
            return std::nullopt;
        }
        tasks.push_back(std::move(*task));
    }
    return tasks;
}

} // namespace stukit::tasks
