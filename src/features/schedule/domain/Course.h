#pragma once

#include <QString>
#include <QUuid>

#include <optional>

namespace stukit::schedule {

struct CourseData
{
    QUuid id;
    QUuid termId;
    QString name;
    QString teacher;
    QString location;
    QString color;
};

class Course final
{
public:
    static constexpr qsizetype MaximumNameLength = 80;
    static constexpr qsizetype MaximumTeacherLength = 60;
    static constexpr qsizetype MaximumLocationLength = 80;

    static std::optional<Course> create(CourseData data);

    [[nodiscard]] const QUuid &id() const noexcept;
    [[nodiscard]] const QUuid &termId() const noexcept;
    [[nodiscard]] const QString &name() const noexcept;
    [[nodiscard]] const QString &teacher() const noexcept;
    [[nodiscard]] const QString &location() const noexcept;
    [[nodiscard]] const QString &color() const noexcept;

private:
    explicit Course(CourseData data);

    CourseData m_data;
};

} // namespace stukit::schedule
