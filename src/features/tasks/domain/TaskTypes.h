#pragma once

namespace stukit::tasks {

enum class TaskPriority
{
    Low,
    Normal,
    High,
    Urgent,
};

enum class TaskStatus
{
    Pending,
    Completed,
    Archived,
};

} // namespace stukit::tasks
