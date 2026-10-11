#pragma once

namespace stukit::schedule {

enum class WeekPattern
{
    EveryWeek,
    OddWeeks,
    EvenWeeks
};

[[nodiscard]] constexpr bool isValidWeekPattern(WeekPattern pattern) noexcept
{
    switch (pattern) {
    case WeekPattern::EveryWeek:
    case WeekPattern::OddWeeks:
    case WeekPattern::EvenWeeks:
        return true;
    }
    return false;
}

[[nodiscard]] constexpr bool matchesWeek(
    WeekPattern pattern, int week) noexcept
{
    if (week < 1 || !isValidWeekPattern(pattern)) {
        return false;
    }
    switch (pattern) {
    case WeekPattern::EveryWeek:
        return true;
    case WeekPattern::OddWeeks:
        return week % 2 == 1;
    case WeekPattern::EvenWeeks:
        return week % 2 == 0;
    }
    return false;
}

} // namespace stukit::schedule
