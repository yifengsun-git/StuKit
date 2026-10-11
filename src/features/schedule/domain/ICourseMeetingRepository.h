#pragma once

#include "CourseMeeting.h"

#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::schedule {

class ICourseMeetingRepository
{
public:
    virtual ~ICourseMeetingRepository() = default;

    [[nodiscard]] virtual bool save(const CourseMeeting &meeting) = 0;
    [[nodiscard]] virtual std::optional<CourseMeeting> findById(
        const QUuid &id) const = 0;
    [[nodiscard]] virtual std::optional<std::vector<CourseMeeting>>
    findByCourseId(const QUuid &courseId) const = 0;
    [[nodiscard]] virtual std::optional<std::vector<CourseMeeting>> findAll()
        const = 0;
};

} // namespace stukit::schedule
