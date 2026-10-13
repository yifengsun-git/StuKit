#pragma once

#include "AcademicTerm.h"
#include "Course.h"
#include "CourseMeeting.h"

#include <QString>
#include <QUuid>

namespace stukit::schedule::testing {

inline QString uniqueScheduleConnectionName(const QString &purpose)
{
    return QStringLiteral("stukit-schedule-%1-%2")
        .arg(purpose, QUuid::createUuid().toString(QUuid::WithoutBraces));
}

inline AcademicTerm makeTerm(
    const QUuid &id,
    const QString &name = QStringLiteral("2026 秋季学期"),
    const QDate &startDate = QDate(2026, 9, 7),
    int totalWeeks = 18,
    bool archived = false)
{
    return AcademicTerm::create(AcademicTermData{
        .id = id,
        .name = name,
        .startDate = startDate,
        .totalWeeks = totalWeeks,
        .archived = archived,
    }).value();
}

inline Course makeCourse(
    const QUuid &id,
    const QUuid &termId,
    const QString &name = QStringLiteral("高等数学"),
    const QString &teacher = QStringLiteral("王老师"),
    const QString &location = QStringLiteral("A101"),
    const QString &color = QStringLiteral("#3B82F6"),
    const QString &assessmentStandard = {},
    const QString &teachingPlan = {})
{
    return Course::create(CourseData{
        .id = id,
        .termId = termId,
        .name = name,
        .teacher = teacher,
        .location = location,
        .assessmentStandard = assessmentStandard,
        .teachingPlan = teachingPlan,
        .color = color,
    }).value();
}

inline CourseMeeting makeMeeting(
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
