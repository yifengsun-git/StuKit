#include "CourseMeeting.h"

#include <algorithm>
#include <utility>

namespace stukit::schedule {
namespace {

std::vector<int> expandedWeeks(const CourseMeetingData &data)
{
    std::vector<int> weeks;
    for (int week = data.startWeek; week <= data.endWeek; ++week) {
        if (matchesWeek(data.weekPattern, week)) {
            weeks.push_back(week);
        }
    }
    return weeks;
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
        || !isValidWeekPattern(data.weekPattern)) {
        return std::nullopt;
    }

    if (data.activeWeeks.empty()) {
        data.activeWeeks = expandedWeeks(data);
    } else {
        std::ranges::sort(data.activeWeeks);
        const auto uniqueEnd = std::ranges::unique(data.activeWeeks).begin();
        data.activeWeeks.erase(uniqueEnd, data.activeWeeks.end());
    }
    if (data.activeWeeks.empty()) {
        return std::nullopt;
    }
    if (data.activeWeeks.front() < MinimumWeek
        || data.activeWeeks.back() > MaximumWeek) {
        return std::nullopt;
    }
    data.startWeek = data.activeWeeks.front();
    data.endWeek = data.activeWeeks.back();

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

const std::vector<int> &CourseMeeting::activeWeeks() const noexcept
{
    return m_data.activeWeeks;
}

bool CourseMeeting::occursInWeek(int week) const noexcept
{
    return std::ranges::binary_search(m_data.activeWeeks, week);
}

} // namespace stukit::schedule
