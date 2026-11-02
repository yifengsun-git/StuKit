#pragma once

#include "Money.h"

#include <QUuid>

#include <optional>

namespace stukit::accounting {

struct MonthlyBudgetData
{
    QUuid categoryId;
    int year{0};
    int month{0};
    Money limit{Money::fromCents(0)};
};

class MonthlyBudget final
{
public:
    [[nodiscard]] static std::optional<MonthlyBudget> create(
        MonthlyBudgetData data);

    [[nodiscard]] const QUuid &categoryId() const noexcept;
    [[nodiscard]] int year() const noexcept;
    [[nodiscard]] int month() const noexcept;
    [[nodiscard]] Money limit() const noexcept;

private:
    explicit MonthlyBudget(MonthlyBudgetData data);

    MonthlyBudgetData m_data;
};

} // namespace stukit::accounting
