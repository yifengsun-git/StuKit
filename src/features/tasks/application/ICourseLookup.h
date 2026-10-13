#pragma once

#include <QString>
#include <QUuid>

#include <variant>
#include <vector>

namespace stukit::tasks {

struct CourseReference
{
    QUuid id;
    QString name;
    QUuid termId;
    QString termName;
};

enum class CourseLookupError
{
    NotFound,
    StorageFailure,
};

using FindCourseResult = std::variant<CourseReference, CourseLookupError>;
using AvailableCoursesResult = std::variant<
    std::vector<CourseReference>, CourseLookupError>;

class ICourseLookup
{
public:
    virtual ~ICourseLookup() = default;

    [[nodiscard]] virtual FindCourseResult findCourse(
        const QUuid &id) const = 0;
    [[nodiscard]] virtual AvailableCoursesResult availableCourses() const = 0;
};

} // namespace stukit::tasks
