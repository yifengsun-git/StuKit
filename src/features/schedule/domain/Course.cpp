#include "Course.h"

#include <utility>

namespace stukit::schedule {
namespace {

bool isHexColor(const QString &color)
{
    if (color.size() != 7 || color.front() != QLatin1Char('#')) {
        return false;
    }
    for (qsizetype index = 1; index < color.size(); ++index) {
        const QChar character = color.at(index);
        const bool isDigit = character >= QLatin1Char('0')
            && character <= QLatin1Char('9');
        const QChar upper = character.toUpper();
        const bool isHexLetter = upper >= QLatin1Char('A')
            && upper <= QLatin1Char('F');
        if (!isDigit && !isHexLetter) {
            return false;
        }
    }
    return true;
}

} // namespace

std::optional<Course> Course::create(CourseData data)
{
    data.name = data.name.trimmed();
    data.teacher = data.teacher.trimmed();
    data.location = data.location.trimmed();
    data.assessmentStandard = data.assessmentStandard.trimmed();
    data.teachingPlan = data.teachingPlan.trimmed();
    data.color = data.color.trimmed().toUpper();

    if (data.id.isNull()
        || data.termId.isNull()
        || data.name.isEmpty()
        || data.name.size() > MaximumNameLength
        || data.teacher.size() > MaximumTeacherLength
        || data.location.size() > MaximumLocationLength
        || data.assessmentStandard.size() > MaximumAssessmentStandardLength
        || data.teachingPlan.size() > MaximumTeachingPlanLength
        || !isHexColor(data.color)) {
        return std::nullopt;
    }

    return Course(std::move(data));
}

Course::Course(CourseData data)
    : m_data(std::move(data))
{
}

const QUuid &Course::id() const noexcept
{
    return m_data.id;
}

const QUuid &Course::termId() const noexcept
{
    return m_data.termId;
}

const QString &Course::name() const noexcept
{
    return m_data.name;
}

const QString &Course::teacher() const noexcept
{
    return m_data.teacher;
}

const QString &Course::location() const noexcept
{
    return m_data.location;
}

const QString &Course::assessmentStandard() const noexcept
{
    return m_data.assessmentStandard;
}

const QString &Course::teachingPlan() const noexcept
{
    return m_data.teachingPlan;
}

const QString &Course::color() const noexcept
{
    return m_data.color;
}

} // namespace stukit::schedule
