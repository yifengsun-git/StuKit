#pragma once

#include "IAcademicTermRepository.h"
#include "ICourseMeetingRepository.h"
#include "ICourseRepository.h"

#include <QString>
#include <QUuid>

#include <variant>
#include <vector>

namespace stukit::schedule {

struct WeeklyScheduleItem
{
    QUuid courseId;
    QUuid meetingId;
    QString courseName;
    QString teacher;
    QString location;
    QString color;
    Weekday weekday{Weekday::Monday};
    int startPeriod{0};
    int endPeriod{0};
};

struct WeeklySchedule
{
    AcademicTerm term;
    int week{0};
    std::vector<WeeklyScheduleItem> items;
};

enum class GetWeeklyScheduleError
{
    InvalidInput,
    TermNotFound,
    WeekOutOfRange,
    StorageFailure
};

using GetWeeklyScheduleResult = std::variant<
    WeeklySchedule, GetWeeklyScheduleError>;

class GetWeeklySchedule final
{
public:
    GetWeeklySchedule(
        IAcademicTermRepository &termRepository,
        ICourseRepository &courseRepository,
        ICourseMeetingRepository &meetingRepository);

    [[nodiscard]] GetWeeklyScheduleResult execute(
        const QUuid &termId, int week) const;

private:
    IAcademicTermRepository &m_termRepository;
    ICourseRepository &m_courseRepository;
    ICourseMeetingRepository &m_meetingRepository;
};

} // namespace stukit::schedule
