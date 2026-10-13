#include "SqliteReminderDeliveryStore.h"

#include <QSqlQuery>

#include <utility>

namespace stukit::tasks {

SqliteReminderDeliveryStore::SqliteReminderDeliveryStore(QSqlDatabase database)
    : m_database(std::move(database))
{
}

bool SqliteReminderDeliveryStore::wasDelivered(
    const QUuid &taskId, const QDateTime &reminderAtUtc) const
{
    if (!m_database.isOpen() || taskId.isNull() || !reminderAtUtc.isValid()) {
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT 1 FROM task_reminder_deliveries "
        "WHERE task_id = :taskId AND reminder_at = :reminderAt"));
    query.bindValue(QStringLiteral(":taskId"),
                    taskId.toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":reminderAt"),
                    reminderAtUtc.toString(Qt::ISODateWithMs));
    return query.exec() && query.next();
}

bool SqliteReminderDeliveryStore::markDelivered(
    const QUuid &taskId,
    const QDateTime &reminderAtUtc,
    const QDateTime &deliveredAtUtc)
{
    if (!m_database.isOpen() || taskId.isNull()) return false;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO task_reminder_deliveries("
        "task_id, reminder_at, delivered_at) "
        "VALUES(:taskId, :reminderAt, :deliveredAt)"));
    query.bindValue(QStringLiteral(":taskId"),
                    taskId.toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":reminderAt"),
                    reminderAtUtc.toString(Qt::ISODateWithMs));
    query.bindValue(QStringLiteral(":deliveredAt"),
                    deliveredAtUtc.toString(Qt::ISODateWithMs));
    return query.exec();
}

} // namespace stukit::tasks
