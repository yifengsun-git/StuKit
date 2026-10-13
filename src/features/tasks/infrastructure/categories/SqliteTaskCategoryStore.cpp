#include "SqliteTaskCategoryStore.h"

#include <QDateTime>
#include <QSqlQuery>
#include <QTimeZone>

#include <utility>

namespace stukit::tasks {

SqliteTaskCategoryStore::SqliteTaskCategoryStore(QSqlDatabase database)
    : m_database(std::move(database))
{
}

std::optional<QStringList> SqliteTaskCategoryStore::categories() const
{
    if (!m_database.isOpen()) return std::nullopt;
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT name FROM task_categories ORDER BY sort_order, name"))) {
        return std::nullopt;
    }
    QStringList result;
    while (query.next()) result.push_back(query.value(0).toString());
    return result;
}

bool SqliteTaskCategoryStore::addCategory(const QString &name)
{
    const QString normalized = name.trimmed();
    if (!m_database.isOpen() || normalized.isEmpty() || normalized.size() > 40) {
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO task_categories(name, sort_order, created_at) "
        "VALUES(:name, "
        "COALESCE((SELECT MAX(sort_order) + 1 FROM task_categories), 0), "
        ":createdAt)"));
    query.bindValue(QStringLiteral(":name"), normalized);
    query.bindValue(QStringLiteral(":createdAt"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    return query.exec();
}

} // namespace stukit::tasks
