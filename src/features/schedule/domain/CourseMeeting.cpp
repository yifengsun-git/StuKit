#include "CourseMeeting.h"

#include <utility>

namespace stukit::schedule {
namespace {

bool hasActiveWeek(const CourseMeetingData &data)
{
    for (int week = data.startWeek; week <= data.endWeek; ++week) {
        if (matchesWeek(data.weekPattern, week)) {
            return true;
        }
    }
    return false;
}

} // namespace

std::optional<CourseMeeting> CourseMeeting::create(CourseMeetingData data)
{
    if (data.id.isNull()
        || data.courseId.isNull()
        || !isValidWeekday(data.weekday)
        || data.startPeriod < MinimumPeriod
        || data.endPeriod > MaximumPeriod
        || data.startPeriod > data.endPeriod
        || data.startWeek < MinimumWeek
        || data.endWeek > MaximumWeek
        || data.startWeek > data.endWeek
        || !isValidWeekPattern(data.weekPattern)
        || !hasActiveWeek(data)) {
        return std::nullopt;
    }

    return CourseMeeting(std::move(data));
}

CourseMeeting::CourseMeeting(CourseMeetingData data)
    : m_data(std::move(data))
{
}

const QUuid &CourseMeeting::id() const noexcept
{
    return m_data.id;
}

const QUuid &CourseMeeting::courseId() const noexcept
{
    return m_data.courseId;
}

Weekday CourseMeeting::weekday() const noexcept
{
    return m_data.weekday;
}

int CourseMeeting::startPeriod() const noexcept
{
    return m_data.startPeriod;
}

int CourseMeeting::endPeriod() const noexcept
{
    return m_data.endPeriod;
}

int CourseMeeting::startWeek() const noexcept
{
    return m_data.startWeek;
}

int CourseMeeting::endWeek() const noexcept
{
    return m_data.endWeek;
}

WeekPattern CourseMeeting::weekPattern() const noexcept
{
    return m_data.weekPattern;
}

bool CourseMeeting::occursInWeek(int week) const noexcept
{
    return week >= m_data.startWeek
        && week <= m_data.endWeek
        && matchesWeek(m_data.weekPattern, week);
}

} // namespace stukit::schedule
