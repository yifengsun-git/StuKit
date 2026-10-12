#pragma once

#include "DetectScheduleConflicts.h"

#include <variant>
#include <vector>

namespace stukit::schedule {

struct SaveCourseMeetingRequest
{
    QUuid id;
    QUuid courseId;
    Weekday weekday{Weekday::Monday};
    int startPeriod{0};
    int endPeriod{0};
    int startWeek{0};
    int endWeek{0};
    WeekPattern weekPattern{WeekPattern::EveryWeek};
};

struct CourseMeetingConflicts
{
    std::vector<ScheduleConflict> items;
};

enum class SaveCourseMeetingError
{
    InvalidInput,
    MeetingNotFound,
    CourseNotFound,
    TermNotFound,
    TermArchived,
    MeetingExceedsTerm,
    StorageFailure
};

using SaveCourseMeetingResult = std::variant<
    CourseMeeting, CourseMeetingConflicts, SaveCourseMeetingError>;

class SaveCourseMeeting final
{
public:
    SaveCourseMeeting(
        IAcademicTermRepository &termRepository,
        ICourseRepository &courseRepository,
        ICourseMeetingRepository &meetingRepository);

    [[nodiscard]] SaveCourseMeetingResult execute(
        const SaveCourseMeetingRequest &request) const;

private:
    IAcademicTermRepository &m_termRepository;
    ICourseRepository &m_courseRepository;
    ICourseMeetingRepository &m_meetingRepository;
    DetectScheduleConflicts m_detectConflicts;
};

} // namespace stukit::schedule
