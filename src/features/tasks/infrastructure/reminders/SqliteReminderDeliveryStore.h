#pragma once

#include "IReminderDeliveryStore.h"

#include <QSqlDatabase>

namespace stukit::tasks {

class SqliteReminderDeliveryStore final : public IReminderDeliveryStore
{
public:
    explicit SqliteReminderDeliveryStore(QSqlDatabase database);
    [[nodiscard]] bool wasDelivered(
        const QUuid &taskId, const QDateTime &reminderAtUtc) const override;
    [[nodiscard]] bool markDelivered(
        const QUuid &taskId,
        const QDateTime &reminderAtUtc,
        const QDateTime &deliveredAtUtc) override;

private:
    QSqlDatabase m_database;
};

} // namespace stukit::tasks
