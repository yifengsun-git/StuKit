#pragma once

namespace stukit::schedule {

enum class Weekday
{
    Monday = 1,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday
};

[[nodiscard]] constexpr bool isValidWeekday(Weekday weekday) noexcept
{
    return weekday >= Weekday::Monday && weekday <= Weekday::Sunday;
}

} // namespace stukit::schedule
