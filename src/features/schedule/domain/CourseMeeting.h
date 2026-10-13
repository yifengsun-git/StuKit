#pragma once

#include "Weekday.h"
#include "WeekPattern.h"

#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::schedule {

struct CourseMeetingData
{
    QUuid id;
    QUuid courseId;
    Weekday weekday{Weekday::Monday};
    int startPeriod{0};
    int endPeriod{0};
    int startWeek{0};
    int endWeek{0};
    WeekPattern weekPattern{WeekPattern::EveryWeek};
    std::vector<int> activeWeeks;
};

class CourseMeeting final
{
public:
    static constexpr int MinimumPeriod = 1;
    static constexpr int MaximumPeriod = 20;
    static constexpr int MinimumWeek = 1;
    static constexpr int MaximumWeek = 30;

    static std::optional<CourseMeeting> create(CourseMeetingData data);

    [[nodiscard]] const QUuid &id() const noexcept;
    [[nodiscard]] const QUuid &courseId() const noexcept;
    [[nodiscard]] Weekday weekday() const noexcept;
    [[nodiscard]] int startPeriod() const noexcept;
    [[nodiscard]] int endPeriod() const noexcept;
    [[nodiscard]] int startWeek() const noexcept;
    [[nodiscard]] int endWeek() const noexcept;
    [[nodiscard]] WeekPattern weekPattern() const noexcept;
    [[nodiscard]] const std::vector<int> &activeWeeks() const noexcept;
    [[nodiscard]] bool occursInWeek(int week) const noexcept;

private:
    explicit CourseMeeting(CourseMeetingData data);

    CourseMeetingData m_data;
};

} // namespace stukit::schedule
