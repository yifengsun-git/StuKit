#include "AcademicTerm.h"

#include <utility>

namespace stukit::schedule {

std::optional<AcademicTerm> AcademicTerm::create(AcademicTermData data)
{
    data.name = data.name.trimmed();
    if (data.id.isNull()
        || data.name.isEmpty()
        || data.name.size() > MaximumNameLength
        || !data.startDate.isValid()
        || data.totalWeeks < MinimumTotalWeeks
        || data.totalWeeks > MaximumTotalWeeks) {
        return std::nullopt;
    }

    return AcademicTerm(std::move(data));
}

AcademicTerm::AcademicTerm(AcademicTermData data)
    : m_data(std::move(data))
{
}

const QUuid &AcademicTerm::id() const noexcept
{
    return m_data.id;
}

const QString &AcademicTerm::name() const noexcept
{
    return m_data.name;
}

QDate AcademicTerm::startDate() const noexcept
{
    return m_data.startDate;
}

int AcademicTerm::totalWeeks() const noexcept
{
    return m_data.totalWeeks;
}

bool AcademicTerm::isArchived() const noexcept
{
    return m_data.archived;
}

} // namespace stukit::schedule
