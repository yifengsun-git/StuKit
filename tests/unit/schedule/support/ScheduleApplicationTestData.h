#pragma once

#include "AcademicTerm.h"
#include "Course.h"
#include "CourseMeeting.h"

namespace stukit::schedule::testing {

inline AcademicTerm applicationTestTerm(
    const QUuid &id,
    int totalWeeks = 18,
    bool archived = false)
{
    return AcademicTerm::create(AcademicTermData{
        .id = id,
        .name = QStringLiteral("测试学期"),
        .startDate = QDate(2026, 9, 7),
        .totalWeeks = totalWeeks,
        .archived = archived,
    }).value();
}

inline Course applicationTestCourse(
    const QUuid &id,
    const QUuid &termId,
    const QString &name = QStringLiteral("高等数学"))
{
    return Course::create(CourseData{
        .id = id,
        .termId = termId,
        .name = name,
        .teacher = QStringLiteral("王老师"),
        .location = QStringLiteral("A101"),
        .color = QStringLiteral("#3B82F6"),
    }).value();
}

inline CourseMeeting applicationTestMeeting(
    const QUuid &id,
    const QUuid &courseId,
    Weekday weekday = Weekday::Monday,
    int startPeriod = 1,
    int endPeriod = 2,
    int startWeek = 1,
    int endWeek = 18,
    WeekPattern pattern = WeekPattern::EveryWeek)
{
    return CourseMeeting::create(CourseMeetingData{
        .id = id,
        .courseId = courseId,
        .weekday = weekday,
        .startPeriod = startPeriod,
        .endPeriod = endPeriod,
        .startWeek = startWeek,
        .endWeek = endWeek,
        .weekPattern = pattern,
    }).value();
}

} // namespace stukit::schedule::testing
