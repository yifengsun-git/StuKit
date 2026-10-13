#include "ScanTaskReminders.h"

namespace stukit::tasks {

ScanTaskReminders::ScanTaskReminders(
    ITaskRepository &repository,
    IReminderDeliveryStore &deliveryStore,
    ITaskReminderNotifier &notifier)
    : m_repository(repository)
    , m_deliveryStore(deliveryStore)
    , m_notifier(notifier)
{
}

std::variant<int, ScanTaskRemindersError> ScanTaskReminders::execute(
    const QDateTime &nowUtc) const
{
    if (!nowUtc.isValid() || nowUtc.timeSpec() != Qt::UTC) {
        return ScanTaskRemindersError::InvalidTime;
    }
    const auto tasks = m_repository.findAll();
    if (!tasks) return ScanTaskRemindersError::StorageFailure;
    int delivered = 0;
    for (const TaskItem &task : *tasks) {
        if (!task.reminderIsDue(nowUtc)
            || !task.dueAtUtc() || !task.reminderAtUtc()
            || m_deliveryStore.wasDelivered(task.id(), *task.reminderAtUtc())) {
            continue;
        }
        const TaskReminder reminder{
            .taskId = task.id(),
            .title = task.title(),
            .courseName = task.courseNameSnapshot(),
            .dueAtUtc = *task.dueAtUtc(),
            .reminderAtUtc = *task.reminderAtUtc(),
        };
        if (!m_notifier.deliver(reminder)
            || !m_deliveryStore.markDelivered(
                task.id(), *task.reminderAtUtc(), nowUtc)) {
            return ScanTaskRemindersError::DeliveryFailure;
        }
        ++delivered;
    }
    return delivered;
}

} // namespace stukit::tasks
