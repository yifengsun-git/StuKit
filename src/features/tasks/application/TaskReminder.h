#pragma once

#include <QDateTime>
#include <QString>
#include <QUuid>

namespace stukit::tasks {

struct TaskReminder
{
    QUuid taskId;
    QString title;
    QString courseName;
    QDateTime dueAtUtc;
    QDateTime reminderAtUtc;
};

} // namespace stukit::tasks
