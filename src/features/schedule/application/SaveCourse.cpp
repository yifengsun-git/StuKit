#include "SaveCourse.h"

#include <algorithm>

namespace stukit::schedule {

SaveCourse::SaveCourse(
    IAcademicTermRepository &termRepository,
    ICourseRepository &courseRepository)
    : m_termRepository(termRepository)
    , m_courseRepository(courseRepository)
{
}

SaveCourseResult SaveCourse::execute(const SaveCourseRequest &request) const
{
    const bool isCreating = request.id.isNull();
    const QUuid id = isCreating ? QUuid::createUuid() : request.id;
    const auto course = Course::create(CourseData{
        .id = id,
        .termId = request.termId,
        .name = request.name,
        .teacher = request.teacher,
        .location = request.location,
        .assessmentStandard = request.assessmentStandard,
        .teachingPlan = request.teachingPlan,
        .color = request.color,
    });
    if (!course.has_value()) {
        return SaveCourseError::InvalidInput;
    }

    const auto terms = m_termRepository.findAll();
    if (!terms.has_value()) {
        return SaveCourseError::StorageFailure;
    }
    const auto term = std::ranges::find_if(
        *terms,
        [&request](const AcademicTerm &candidate) {
            return candidate.id() == request.termId;
        });
    if (term == terms->cend()) {
        return SaveCourseError::TermNotFound;
    }
    if (term->isArchived()) {
        return SaveCourseError::TermArchived;
    }

    if (!isCreating) {
        const auto courses = m_courseRepository.findAll();
        if (!courses.has_value()) {
            return SaveCourseError::StorageFailure;
        }
        const auto existing = std::ranges::find_if(
            *courses,
            [&id](const Course &candidate) {
                return candidate.id() == id;
            });
        if (existing == courses->cend()) {
            return SaveCourseError::CourseNotFound;
        }
        if (existing->termId() != request.termId) {
            return SaveCourseError::TermCannotChange;
        }
    }

    if (!m_courseRepository.save(*course)) {
        return SaveCourseError::StorageFailure;
    }
    return *course;
}

} // namespace stukit::schedule
