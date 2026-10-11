#pragma once

#include <QDate>
#include <QString>
#include <QUuid>

#include <optional>

namespace stukit::schedule {

struct AcademicTermData
{
    QUuid id;
    QString name;
    QDate startDate;
    int totalWeeks{0};
    bool archived{false};
};

class AcademicTerm final
{
public:
    static constexpr qsizetype MaximumNameLength = 40;
    static constexpr int MinimumTotalWeeks = 1;
    static constexpr int MaximumTotalWeeks = 30;

    static std::optional<AcademicTerm> create(AcademicTermData data);

    [[nodiscard]] const QUuid &id() const noexcept;
    [[nodiscard]] const QString &name() const noexcept;
    [[nodiscard]] QDate startDate() const noexcept;
    [[nodiscard]] int totalWeeks() const noexcept;
    [[nodiscard]] bool isArchived() const noexcept;

private:
    explicit AcademicTerm(AcademicTermData data);

    AcademicTermData m_data;
};

} // namespace stukit::schedule
