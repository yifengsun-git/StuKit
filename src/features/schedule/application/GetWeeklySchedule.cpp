#include "GetWeeklySchedule.h"

#include <algorithm>

namespace stukit::schedule {

GetWeeklySchedule::GetWeeklySchedule(
    IAcademicTermRepository &termRepository,
    ICourseRepository &courseRepository,
    ICourseMeetingRepository &meetingRepository)
    : m_termRepository(termRepository)
    , m_courseRepository(courseRepository)
    , m_meetingRepository(meetingRepository)
{
}

GetWeeklyScheduleResult GetWeeklySchedule::execute(
    const QUuid &termId, int week) const
{
    if (termId.isNull() || week < AcademicTerm::MinimumTotalWeeks) {
        return GetWeeklyScheduleError::InvalidInput;
    }

    const auto terms = m_termRepository.findAll();
    if (!terms.has_value()) {
        return GetWeeklyScheduleError::StorageFailure;
    }
    const auto term = std::ranges::find_if(
        *terms,
        [&termId](const AcademicTerm &candidate) {
            return candidate.id() == termId;
        });
    if (term == terms->cend()) {
        return GetWeeklyScheduleError::TermNotFound;
    }
    if (week > term->totalWeeks()) {
        return GetWeeklyScheduleError::WeekOutOfRange;
    }

    const auto courses = m_courseRepository.findAll();
    const auto meetings = m_meetingRepository.findAll();
    if (!courses.has_value() || !meetings.has_value()) {
        return GetWeeklyScheduleError::StorageFailure;
    }

    WeeklySchedule schedule{
        .term = *term,
        .week = week,
        .items = {},
    };
    for (const auto &meeting : *meetings) {
        const auto course = std::ranges::find_if(
            *courses,
            [&meeting](const Course &candidate) {
                return candidate.id() == meeting.courseId();
            });
        if (course == courses->cend()) {
            return GetWeeklyScheduleError::StorageFailure;
        }
        if (course->termId() != termId) {
            continue;
        }
        if (meeting.endWeek() > term->totalWeeks()) {
            return GetWeeklyScheduleError::StorageFailure;
        }
        if (!meeting.occursInWeek(week)) {
            continue;
        }

        schedule.items.push_back(WeeklyScheduleItem{
            .courseId = course->id(),
            .meetingId = meeting.id(),
            .courseName = course->name(),
            .teacher = course->teacher(),
            .location = course->location(),
            .color = course->color(),
            .weekday = meeting.weekday(),
            .startPeriod = meeting.startPeriod(),
            .endPeriod = meeting.endPeriod(),
        });
    }

    std::ranges::sort(
        schedule.items,
        [](const WeeklyScheduleItem &left, const WeeklyScheduleItem &right) {
            if (left.weekday != right.weekday) {
                return static_cast<int>(left.weekday)
                    < static_cast<int>(right.weekday);
            }
            if (left.startPeriod != right.startPeriod) {
                return left.startPeriod < right.startPeriod;
            }
            if (left.endPeriod != right.endPeriod) {
                return left.endPeriod < right.endPeriod;
            }
            const int nameComparison = QString::compare(
                left.courseName, right.courseName, Qt::CaseSensitive);
            if (nameComparison != 0) {
                return nameComparison < 0;
            }
            return left.meetingId.toString() < right.meetingId.toString();
        });

    return schedule;
}

} // namespace stukit::schedule
