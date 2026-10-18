#pragma once

#include "IAcademicTermRepository.h"
#include "ICourseMeetingRepository.h"
#include "ICourseRepository.h"

#include <QUuid>

namespace stukit::schedule {

enum class DeleteScheduleItemError
{
    None,
    InvalidInput,
    ItemNotFound,
    RelatedItemNotFound,
    TermArchived,
    TermNotArchived,
    StorageFailure
};

class DeleteScheduleItem final
{
public:
    DeleteScheduleItem(
        IAcademicTermRepository &termRepository,
        ICourseRepository &courseRepository,
        ICourseMeetingRepository &meetingRepository);

    [[nodiscard]] DeleteScheduleItemError deleteMeeting(
        const QUuid &meetingId) const;
    [[nodiscard]] DeleteScheduleItemError deleteCourse(
        const QUuid &courseId) const;
    [[nodiscard]] DeleteScheduleItemError deleteTerm(
        const QUuid &termId) const;

    static constexpr DeleteScheduleItemError Success =
        DeleteScheduleItemError::None;

private:
    IAcademicTermRepository &m_termRepository;
    ICourseRepository &m_courseRepository;
    ICourseMeetingRepository &m_meetingRepository;
};

} // namespace stukit::schedule
