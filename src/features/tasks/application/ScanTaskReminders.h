#pragma once

#include "IReminderDeliveryStore.h"
#include "ITaskReminderNotifier.h"
#include "ITaskRepository.h"

#include <variant>

namespace stukit::tasks {

enum class ScanTaskRemindersError { InvalidTime, StorageFailure, DeliveryFailure };

class ScanTaskReminders final
{
public:
    ScanTaskReminders(
        ITaskRepository &repository,
        IReminderDeliveryStore &deliveryStore,
        ITaskReminderNotifier &notifier);

    [[nodiscard]] std::variant<int, ScanTaskRemindersError> execute(
        const QDateTime &nowUtc) const;

private:
    ITaskRepository &m_repository;
    IReminderDeliveryStore &m_deliveryStore;
    ITaskReminderNotifier &m_notifier;
};

} // namespace stukit::tasks
