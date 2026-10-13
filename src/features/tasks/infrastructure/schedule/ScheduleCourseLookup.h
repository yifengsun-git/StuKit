#pragma once

#include "IAcademicTermRepository.h"
#include "ICourseLookup.h"
#include "ICourseRepository.h"

namespace stukit::tasks {

class ScheduleCourseLookup final : public ICourseLookup
{
public:
    ScheduleCourseLookup(
        schedule::IAcademicTermRepository &termRepository,
        schedule::ICourseRepository &courseRepository);

    [[nodiscard]] FindCourseResult findCourse(
        const QUuid &id) const override;
    [[nodiscard]] AvailableCoursesResult availableCourses() const override;

private:
    schedule::IAcademicTermRepository &m_termRepository;
    schedule::ICourseRepository &m_courseRepository;
};

} // namespace stukit::tasks
