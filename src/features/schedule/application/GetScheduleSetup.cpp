#include "GetScheduleSetup.h"

#include <utility>

namespace stukit::schedule {

GetScheduleSetup::GetScheduleSetup(
    IAcademicTermRepository &termRepository,
    ICourseRepository &courseRepository,
    ICourseMeetingRepository &meetingRepository)
    : m_termRepository(termRepository)
    , m_courseRepository(courseRepository)
    , m_meetingRepository(meetingRepository)
{
}

GetScheduleSetupResult GetScheduleSetup::execute() const
{
    auto terms = m_termRepository.findAll();
    auto courses = m_courseRepository.findAll();
    auto meetings = m_meetingRepository.findAll();
    if (!terms.has_value() || !courses.has_value()
        || !meetings.has_value()) {
        return GetScheduleSetupError::StorageFailure;
    }

    return ScheduleSetup{
        .terms = std::move(*terms),
        .courses = std::move(*courses),
        .meetings = std::move(*meetings),
    };
}

} // namespace stukit::schedule
