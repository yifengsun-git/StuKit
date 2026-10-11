#include "SaveAcademicTerm.h"

#include <QSet>

#include <algorithm>

namespace stukit::schedule {

SaveAcademicTerm::SaveAcademicTerm(
    IAcademicTermRepository &termRepository,
    ICourseRepository &courseRepository,
    ICourseMeetingRepository &meetingRepository)
    : m_termRepository(termRepository)
    , m_courseRepository(courseRepository)
    , m_meetingRepository(meetingRepository)
{
}

SaveAcademicTermResult SaveAcademicTerm::execute(
    const SaveAcademicTermRequest &request) const
{
    const bool isCreating = request.id.isNull();
    const QUuid id = isCreating ? QUuid::createUuid() : request.id;

    const auto term = AcademicTerm::create(AcademicTermData{
        .id = id,
        .name = request.name,
        .startDate = request.startDate,
        .totalWeeks = request.totalWeeks,
        .archived = request.archived,
    });
    if (!term.has_value()) {
        return SaveAcademicTermError::InvalidInput;
    }

    if (!isCreating) {
        const auto terms = m_termRepository.findAll();
        if (!terms.has_value()) {
            return SaveAcademicTermError::StorageFailure;
        }
        const auto existing = std::ranges::find_if(
            *terms,
            [&id](const AcademicTerm &candidate) {
                return candidate.id() == id;
            });
        if (existing == terms->cend()) {
            return SaveAcademicTermError::NotFound;
        }

        if (term->totalWeeks() < existing->totalWeeks()) {
            const auto courses = m_courseRepository.findAll();
            const auto meetings = m_meetingRepository.findAll();
            if (!courses.has_value() || !meetings.has_value()) {
                return SaveAcademicTermError::StorageFailure;
            }

            QSet<QUuid> courseIds;
            for (const auto &course : *courses) {
                if (course.termId() == id) {
                    courseIds.insert(course.id());
                }
            }
            for (const auto &meeting : *meetings) {
                if (courseIds.contains(meeting.courseId())
                    && meeting.endWeek() > term->totalWeeks()) {
                    return SaveAcademicTermError::HasOutOfRangeMeetings;
                }
            }
        }
    }

    if (!m_termRepository.save(*term)) {
        return SaveAcademicTermError::StorageFailure;
    }
    return *term;
}

} // namespace stukit::schedule
