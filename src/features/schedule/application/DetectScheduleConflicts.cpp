#include "DetectScheduleConflicts.h"

#include <algorithm>

namespace stukit::schedule {
namespace {

template<typename Item>
auto findById(const std::vector<Item> &items, const QUuid &id)
{
    return std::find_if(
        items.cbegin(),
        items.cend(),
        [&id](const Item &item) {
            return item.id() == id;
        });
}

int firstConflictingWeek(
    const CourseMeeting &candidate, const CourseMeeting &existing) noexcept
{
    if (candidate.weekday() != existing.weekday()
        || candidate.startPeriod() > existing.endPeriod()
        || existing.startPeriod() > candidate.endPeriod()) {
        return 0;
    }

    const int firstWeek = std::max(
        candidate.startWeek(), existing.startWeek());
    const int lastWeek = std::min(candidate.endWeek(), existing.endWeek());
    for (int week = firstWeek; week <= lastWeek; ++week) {
        if (candidate.occursInWeek(week) && existing.occursInWeek(week)) {
            return week;
        }
    }
    return 0;
}

} // namespace

DetectScheduleConflicts::DetectScheduleConflicts(
    IAcademicTermRepository &termRepository,
    ICourseRepository &courseRepository,
    ICourseMeetingRepository &meetingRepository)
    : m_termRepository(termRepository)
    , m_courseRepository(courseRepository)
    , m_meetingRepository(meetingRepository)
{
}

DetectScheduleConflictsResult DetectScheduleConflicts::execute(
    const DetectScheduleConflictsRequest &request) const
{
    const auto candidate = CourseMeeting::create(CourseMeetingData{
        .id = request.meetingId,
        .courseId = request.courseId,
        .weekday = request.weekday,
        .startPeriod = request.startPeriod,
        .endPeriod = request.endPeriod,
        .startWeek = request.startWeek,
        .endWeek = request.endWeek,
        .weekPattern = request.weekPattern,
    });
    if (!candidate.has_value()) {
        return DetectScheduleConflictsError::InvalidInput;
    }

    const auto terms = m_termRepository.findAll();
    const auto courses = m_courseRepository.findAll();
    const auto meetings = m_meetingRepository.findAll();
    if (!terms.has_value() || !courses.has_value() || !meetings.has_value()) {
        return DetectScheduleConflictsError::StorageFailure;
    }

    const auto candidateCourse = findById(*courses, candidate->courseId());
    if (candidateCourse == courses->cend()) {
        return DetectScheduleConflictsError::CourseNotFound;
    }

    const auto candidateTerm = findById(*terms, candidateCourse->termId());
    if (candidateTerm == terms->cend()) {
        return DetectScheduleConflictsError::TermNotFound;
    }
    if (candidate->endWeek() > candidateTerm->totalWeeks()) {
        return DetectScheduleConflictsError::MeetingExceedsTerm;
    }

    std::vector<ScheduleConflict> conflicts;
    for (const auto &meeting : *meetings) {
        if (meeting.id() == candidate->id()) {
            continue;
        }

        const auto course = findById(*courses, meeting.courseId());
        if (course == courses->cend()) {
            return DetectScheduleConflictsError::StorageFailure;
        }
        if (course->termId() != candidateTerm->id()) {
            continue;
        }

        const int conflictingWeek = firstConflictingWeek(*candidate, meeting);
        if (conflictingWeek > 0) {
            conflicts.push_back(ScheduleConflict{
                .course = *course,
                .meeting = meeting,
                .firstConflictingWeek = conflictingWeek,
            });
        }
    }

    std::ranges::sort(
        conflicts,
        [](const ScheduleConflict &left, const ScheduleConflict &right) {
            if (left.firstConflictingWeek != right.firstConflictingWeek) {
                return left.firstConflictingWeek < right.firstConflictingWeek;
            }
            if (left.meeting.startPeriod() != right.meeting.startPeriod()) {
                return left.meeting.startPeriod() < right.meeting.startPeriod();
            }
            const int nameComparison = QString::compare(
                left.course.name(), right.course.name(), Qt::CaseSensitive);
            if (nameComparison != 0) {
                return nameComparison < 0;
            }
            return left.meeting.id().toString()
                < right.meeting.id().toString();
        });

    return conflicts;
}

} // namespace stukit::schedule
