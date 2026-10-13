#include "ScheduleCourseLookup.h"

#include <algorithm>

namespace stukit::tasks {

ScheduleCourseLookup::ScheduleCourseLookup(
    schedule::IAcademicTermRepository &termRepository,
    schedule::ICourseRepository &courseRepository)
    : m_termRepository(termRepository)
    , m_courseRepository(courseRepository)
{
}

AvailableCoursesResult ScheduleCourseLookup::availableCourses() const
{
    const auto terms = m_termRepository.findAll();
    const auto courses = m_courseRepository.findAll();
    if (!terms || !courses) {
        return CourseLookupError::StorageFailure;
    }

    std::vector<CourseReference> result;
    for (const schedule::Course &course : *courses) {
        const auto term = std::ranges::find_if(
            *terms,
            [&course](const schedule::AcademicTerm &candidate) {
                return candidate.id() == course.termId();
            });
        if (term == terms->end() || term->isArchived()) {
            continue;
        }
        result.push_back(CourseReference{
            .id = course.id(),
            .name = course.name(),
            .termId = term->id(),
            .termName = term->name(),
        });
    }
    std::ranges::sort(result, {}, [](const CourseReference &reference) {
        return reference.termName + QLatin1Char('\n') + reference.name;
    });
    return result;
}

FindCourseResult ScheduleCourseLookup::findCourse(const QUuid &id) const
{
    if (id.isNull()) {
        return CourseLookupError::NotFound;
    }
    const auto available = availableCourses();
    if (std::holds_alternative<CourseLookupError>(available)) {
        return std::get<CourseLookupError>(available);
    }
    const auto &courses = std::get<std::vector<CourseReference>>(available);
    const auto found = std::ranges::find_if(
        courses,
        [&id](const CourseReference &course) { return course.id == id; });
    return found == courses.end()
        ? FindCourseResult(CourseLookupError::NotFound)
        : FindCourseResult(*found);
}

} // namespace stukit::tasks
