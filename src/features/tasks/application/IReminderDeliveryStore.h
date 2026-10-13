#pragma once

#include <QDateTime>
#include <QUuid>

namespace stukit::tasks {

class IReminderDeliveryStore
{
public:
    virtual ~IReminderDeliveryStore() = default;
    [[nodiscard]] virtual bool wasDelivered(
        const QUuid &taskId, const QDateTime &reminderAtUtc) const = 0;
    [[nodiscard]] virtual bool markDelivered(
        const QUuid &taskId,
        const QDateTime &reminderAtUtc,
        const QDateTime &deliveredAtUtc) = 0;
};

} // namespace stukit::tasks
