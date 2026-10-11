#pragma once

#include "IAcademicTermRepository.h"
#include "ICourseMeetingRepository.h"
#include "ICourseRepository.h"

#include <QUuid>

#include <variant>
#include <vector>

namespace stukit::schedule {

struct DetectScheduleConflictsRequest
{
    QUuid meetingId;
    QUuid courseId;
    Weekday weekday{Weekday::Monday};
    int startPeriod{0};
    int endPeriod{0};
    int startWeek{0};
    int endWeek{0};
    WeekPattern weekPattern{WeekPattern::EveryWeek};
};

struct ScheduleConflict
{
    Course course;
    CourseMeeting meeting;
    int firstConflictingWeek{0};
};

enum class DetectScheduleConflictsError
{
    InvalidInput,
    CourseNotFound,
    TermNotFound,
    MeetingExceedsTerm,
    StorageFailure
};

using DetectScheduleConflictsResult = std::variant<
    std::vector<ScheduleConflict>, DetectScheduleConflictsError>;

class DetectScheduleConflicts final
{
public:
    DetectScheduleConflicts(
        IAcademicTermRepository &termRepository,
        ICourseRepository &courseRepository,
        ICourseMeetingRepository &meetingRepository);

    [[nodiscard]] DetectScheduleConflictsResult execute(
        const DetectScheduleConflictsRequest &request) const;

private:
    IAcademicTermRepository &m_termRepository;
    ICourseRepository &m_courseRepository;
    ICourseMeetingRepository &m_meetingRepository;
};

} // namespace stukit::schedule
