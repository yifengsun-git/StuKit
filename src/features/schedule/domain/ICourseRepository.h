#pragma once

#include "Course.h"

#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::schedule {

class ICourseRepository
{
public:
    virtual ~ICourseRepository() = default;

    [[nodiscard]] virtual bool save(const Course &course) = 0;
    [[nodiscard]] virtual std::optional<Course> findById(
        const QUuid &id) const = 0;
    [[nodiscard]] virtual std::optional<std::vector<Course>> findByTermId(
        const QUuid &termId) const = 0;
    [[nodiscard]] virtual std::optional<std::vector<Course>> findAll() const = 0;
};

} // namespace stukit::schedule
