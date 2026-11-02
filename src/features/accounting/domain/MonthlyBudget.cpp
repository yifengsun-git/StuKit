#include "MonthlyBudget.h"

#include <QDate>

#include <utility>

namespace stukit::accounting {

std::optional<MonthlyBudget> MonthlyBudget::create(MonthlyBudgetData data)
{
    if (data.categoryId.isNull()
        || !QDate(data.year, data.month, 1).isValid()
        || !data.limit.isPositive()) {
        return std::nullopt;
    }
    return MonthlyBudget(std::move(data));
}

const QUuid &MonthlyBudget::categoryId() const noexcept
{
    return m_data.categoryId;
}

int MonthlyBudget::year() const noexcept
{
    return m_data.year;
}

int MonthlyBudget::month() const noexcept
{
    return m_data.month;
}

Money MonthlyBudget::limit() const noexcept
{
    return m_data.limit;
}

MonthlyBudget::MonthlyBudget(MonthlyBudgetData data)
    : m_data(std::move(data))
{
}

} // namespace stukit::accounting
