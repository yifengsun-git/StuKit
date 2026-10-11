#include "SaveCourseMeeting.h"

#include <algorithm>

namespace stukit::schedule {

SaveCourseMeeting::SaveCourseMeeting(
    IAcademicTermRepository &termRepository,
    ICourseRepository &courseRepository,
    ICourseMeetingRepository &meetingRepository)
    : m_meetingRepository(meetingRepository)
    , m_detectConflicts(
          termRepository, courseRepository, meetingRepository)
{
}

SaveCourseMeetingResult SaveCourseMeeting::execute(
    const SaveCourseMeetingRequest &request) const
{
    const bool isCreating = request.id.isNull();
    const QUuid id = isCreating ? QUuid::createUuid() : request.id;

    if (!isCreating) {
        const auto meetings = m_meetingRepository.findAll();
        if (!meetings.has_value()) {
            return SaveCourseMeetingError::StorageFailure;
        }
        const auto existing = std::ranges::find_if(
            *meetings,
            [&id](const CourseMeeting &candidate) {
                return candidate.id() == id;
            });
        if (existing == meetings->cend()) {
            return SaveCourseMeetingError::MeetingNotFound;
        }
    }

    const auto detection = m_detectConflicts.execute(
        DetectScheduleConflictsRequest{
            .meetingId = id,
            .courseId = request.courseId,
            .weekday = request.weekday,
            .startPeriod = request.startPeriod,
            .endPeriod = request.endPeriod,
            .startWeek = request.startWeek,
            .endWeek = request.endWeek,
            .weekPattern = request.weekPattern,
        });
    if (std::holds_alternative<DetectScheduleConflictsError>(detection)) {
        switch (std::get<DetectScheduleConflictsError>(detection)) {
        case DetectScheduleConflictsError::InvalidInput:
            return SaveCourseMeetingError::InvalidInput;
        case DetectScheduleConflictsError::CourseNotFound:
            return SaveCourseMeetingError::CourseNotFound;
        case DetectScheduleConflictsError::TermNotFound:
            return SaveCourseMeetingError::TermNotFound;
        case DetectScheduleConflictsError::MeetingExceedsTerm:
            return SaveCourseMeetingError::MeetingExceedsTerm;
        case DetectScheduleConflictsError::StorageFailure:
            return SaveCourseMeetingError::StorageFailure;
        }
    }

    const auto &conflicts = std::get<std::vector<ScheduleConflict>>(detection);
    if (!conflicts.empty()) {
        return CourseMeetingConflicts{.items = conflicts};
    }

    const auto meeting = CourseMeeting::create(CourseMeetingData{
        .id = id,
        .courseId = request.courseId,
        .weekday = request.weekday,
        .startPeriod = request.startPeriod,
        .endPeriod = request.endPeriod,
        .startWeek = request.startWeek,
        .endWeek = request.endWeek,
        .weekPattern = request.weekPattern,
    });
    if (!meeting.has_value()) {
        return SaveCourseMeetingError::InvalidInput;
    }
    if (!m_meetingRepository.save(*meeting)) {
        return SaveCourseMeetingError::StorageFailure;
    }
    return *meeting;
}

} // namespace stukit::schedule
