#pragma once

#include "PlanningTypes.h"

#include <QDateTime>
#include <QString>
#include <QUuid>

#include <optional>

namespace stukit::planning {

struct PlanningProjectData
{
    QUuid id;
    QString title;
    QString objective;
    QString constraints;
    PlanningProjectStatus status{PlanningProjectStatus::Draft};
    QDateTime createdAtUtc;
    QDateTime updatedAtUtc;
};

class PlanningProject final
{
public:
    static constexpr qsizetype MaximumTitleLength = 120;
    static constexpr qsizetype MaximumObjectiveLength = 4000;
    static constexpr qsizetype MaximumConstraintsLength = 8000;

    static std::optional<PlanningProject> create(PlanningProjectData data);

    [[nodiscard]] const QUuid &id() const noexcept;
    [[nodiscard]] const QString &title() const noexcept;
    [[nodiscard]] const QString &objective() const noexcept;
    [[nodiscard]] const QString &constraints() const noexcept;
    [[nodiscard]] PlanningProjectStatus status() const noexcept;
    [[nodiscard]] const QDateTime &createdAtUtc() const noexcept;
    [[nodiscard]] const QDateTime &updatedAtUtc() const noexcept;
    [[nodiscard]] bool isArchived() const noexcept;
    [[nodiscard]] PlanningProjectData data() const;

private:
    explicit PlanningProject(PlanningProjectData data);

    PlanningProjectData m_data;
};

} // namespace stukit::planning
