#pragma once

#include "IAcademicTermRepository.h"
#include "ICourseMeetingRepository.h"
#include "ICourseRepository.h"

#include <variant>
#include <vector>

namespace stukit::schedule {

struct ScheduleSetup
{
    std::vector<AcademicTerm> terms;
    std::vector<Course> courses;
    std::vector<CourseMeeting> meetings;
};

enum class GetScheduleSetupError
{
    StorageFailure
};

using GetScheduleSetupResult = std::variant<
    ScheduleSetup, GetScheduleSetupError>;

class GetScheduleSetup final
{
public:
    GetScheduleSetup(
        IAcademicTermRepository &termRepository,
        ICourseRepository &courseRepository,
        ICourseMeetingRepository &meetingRepository);

    [[nodiscard]] GetScheduleSetupResult execute() const;

private:
    IAcademicTermRepository &m_termRepository;
    ICourseRepository &m_courseRepository;
    ICourseMeetingRepository &m_meetingRepository;
};

} // namespace stukit::schedule
