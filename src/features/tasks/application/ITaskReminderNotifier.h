#pragma once

#include "TaskReminder.h"

namespace stukit::tasks {

class ITaskReminderNotifier
{
public:
    virtual ~ITaskReminderNotifier() = default;
    [[nodiscard]] virtual bool deliver(const TaskReminder &reminder) = 0;
};

} // namespace stukit::tasks
