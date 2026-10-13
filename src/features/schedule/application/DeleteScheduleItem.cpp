#include "DeleteScheduleItem.h"

#include <algorithm>

namespace stukit::schedule {
namespace {

template<typename Item>
auto findById(const std::vector<Item> &items, const QUuid &id)
{
    return std::ranges::find_if(
        items,
        [&id](const Item &item) {
            return item.id() == id;
        });
}

} // namespace

DeleteScheduleItem::DeleteScheduleItem(
    IAcademicTermRepository &termRepository,
    ICourseRepository &courseRepository,
    ICourseMeetingRepository &meetingRepository)
    : m_termRepository(termRepository)
    , m_courseRepository(courseRepository)
    , m_meetingRepository(meetingRepository)
{
}

DeleteScheduleItemError DeleteScheduleItem::deleteMeeting(
    const QUuid &meetingId) const
{
    if (meetingId.isNull()) {
        return DeleteScheduleItemError::InvalidInput;
    }
    const auto meetings = m_meetingRepository.findAll();
    const auto courses = m_courseRepository.findAll();
    const auto terms = m_termRepository.findAll();
    if (!meetings.has_value() || !courses.has_value() || !terms.has_value()) {
        return DeleteScheduleItemError::StorageFailure;
    }
    const auto meeting = findById(*meetings, meetingId);
    if (meeting == meetings->cend()) {
        return DeleteScheduleItemError::ItemNotFound;
    }
    const auto course = findById(*courses, meeting->courseId());
    if (course == courses->cend()) {
        return DeleteScheduleItemError::RelatedItemNotFound;
    }
    const auto term = findById(*terms, course->termId());
    if (term == terms->cend()) {
        return DeleteScheduleItemError::RelatedItemNotFound;
    }
    if (term->isArchived()) {
        return DeleteScheduleItemError::TermArchived;
    }
    return m_meetingRepository.remove(meetingId)
        ? Success : DeleteScheduleItemError::StorageFailure;
}

DeleteScheduleItemError DeleteScheduleItem::deleteCourse(
    const QUuid &courseId) const
{
    if (courseId.isNull()) {
        return DeleteScheduleItemError::InvalidInput;
    }
    const auto courses = m_courseRepository.findAll();
    const auto terms = m_termRepository.findAll();
    if (!courses.has_value() || !terms.has_value()) {
        return DeleteScheduleItemError::StorageFailure;
    }
    const auto course = findById(*courses, courseId);
    if (course == courses->cend()) {
        return DeleteScheduleItemError::ItemNotFound;
    }
    const auto term = findById(*terms, course->termId());
    if (term == terms->cend()) {
        return DeleteScheduleItemError::RelatedItemNotFound;
    }
    if (term->isArchived()) {
        return DeleteScheduleItemError::TermArchived;
    }
    return m_courseRepository.remove(courseId)
        ? Success : DeleteScheduleItemError::StorageFailure;
}

} // namespace stukit::schedule
