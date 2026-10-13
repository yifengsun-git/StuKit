#pragma once

#include "IAcademicTermRepository.h"
#include "ICourseRepository.h"

#include <QString>
#include <QUuid>

#include <variant>

namespace stukit::schedule {

struct SaveCourseRequest
{
    QUuid id;
    QUuid termId;
    QString name;
    QString teacher;
    QString location;
    QString assessmentStandard;
    QString teachingPlan;
    QString color;
};

enum class SaveCourseError
{
    InvalidInput,
    CourseNotFound,
    TermNotFound,
    TermArchived,
    TermCannotChange,
    StorageFailure
};

using SaveCourseResult = std::variant<Course, SaveCourseError>;

class SaveCourse final
{
public:
    SaveCourse(
        IAcademicTermRepository &termRepository,
        ICourseRepository &courseRepository);

    [[nodiscard]] SaveCourseResult execute(
        const SaveCourseRequest &request) const;

private:
    IAcademicTermRepository &m_termRepository;
    ICourseRepository &m_courseRepository;
};

} // namespace stukit::schedule
